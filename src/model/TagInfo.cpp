// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "taginfo.h"
#include "Log.h"
#include "Utils.h"
#include <QBuffer>
#include <QMessageBox>
#include <QSize>
#include <vector>

// 解析AMF字符串
QString parseAMFString(QDataStream& stream) {
    quint16 strLen;
    stream >> strLen;

    QByteArray strData(strLen, 0);
    stream.readRawData(strData.data(), strLen);

    return QString::fromUtf8(strData);
}

// 解析AMF数字
double parseAMFNumber(QDataStream& stream) {
    double value;
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    stream >> value;
    return value;
}

// 解析AMF布尔值
bool parseAMFBoolean(QDataStream& stream) {
    quint8 value;
    stream >> value;
    return value != 0;
}

// 解析AMF对象或ECMA数组
void DataTagInfo::parseAMFObjectOrArray(QDataStream& stream, MetadataItem& item) {
    // 如果是ECMA数组，跳过数组长度字段
    quint32 arrayLen = UINT32_MAX;
    item.value = static_cast<double>(-1); // 默认无长度

    if (item.type == AMF_ECMA_ARRAY || item.type == AMF_STRICT_ARRAY) {
        stream >> arrayLen;
        item.value = static_cast<double>(arrayLen); // 存储数组长度

        if (item.type == AMF_ECMA_ARRAY) {
            // 对于ECMA数组，结束时会多一个数组结束标记，需要调整长度
            arrayLen++;
        }
    }

    while (arrayLen > 0) {
        arrayLen--;

        // 读取属性名
        QString property_name;
        char property_value = 0;
        int64_t startPos = stream.device()->pos();

        if (item.type != AMF_STRICT_ARRAY) {
            property_name = parseAMFString(stream);

            // 检查是否到达对象结束标记
            if (property_name.isEmpty()) {
                quint8 end_marker;
                stream >> end_marker;
                if (end_marker == AMF_OBJECT_END)
                    break;
                continue;
            }
        }

        // 读取属性值类型
        stream >> property_value;

        // 根据类型解析值
        switch (property_value) {
        case AMF_NUMBER: {
            printLogWithPos(QtDebugMsg,
                            stream,
                            QString("tag[script] amf-info[%1,%2]").arg(property_name).arg((int) property_value));

            double value = parseAMFNumber(stream);
            item.obj_value.emplace_back(
                MetadataItem{property_value, property_name, startPos, static_cast<uint32_t>(stream.device()->pos() - startPos)});
            item.obj_value.back().value = value;
            break;
        }
        case AMF_BOOLEAN: {
            printLogWithPos(QtDebugMsg,
                            stream,
                            QString("tag[script] amf-info[%1,%2]").arg(property_name).arg((int) property_value));

            bool value = parseAMFBoolean(stream);
            item.obj_value.emplace_back(
                MetadataItem{property_value, property_name, startPos, static_cast<uint32_t>(stream.device()->pos() - startPos)});
            item.obj_value.back().value = value;
            break;
        }
        case AMF_STRING: {
            printLogWithPos(QtDebugMsg,
                            stream,
                            QString("tag[script] amf-info[%1,%2]").arg(property_name).arg((int) property_value));

            QString value = parseAMFString(stream);
            item.obj_value.emplace_back(
                MetadataItem{property_value, property_name, startPos, static_cast<uint32_t>(stream.device()->pos() - startPos)});
            item.obj_value.back().value = value.toStdString();
            break;
        }
        case AMF_OBJECT:
        case AMF_ECMA_ARRAY:
        case AMF_STRICT_ARRAY: {
            // 递归解析对象
            printLogWithPos(QtDebugMsg,
                            stream,
                            QString("tag[script] amf-info[%1,%2]").arg(property_name).arg((int) property_value));

            MetadataItem subItem(property_value, property_name, startPos, 0);
            item.obj_value.emplace_back(move(subItem));
            parseAMFObjectOrArray(stream, item.obj_value.back());
            item.obj_value.back().size = stream.device()->pos() - startPos;
            break;
        }
        default: {
            // 对于其他类型，简单跳过
            printLogWithPos(QtDebugMsg,
                            stream,
                            QString("tag[script] amf-info[%1,%2]").arg(property_name).arg((int) property_value));

            item.obj_value.emplace_back(MetadataItem{property_value, property_name});
            item.obj_value.back().value = "null/undefined";
            throw runtime_error("未知类型: " + to_string(property_value));
        }
        }
    }
}

// 解析AMF数据
bool DataTagInfo::ReadFromStream(QDataStream& stream) {
    try {
        int64_t startPos = stream.device()->pos();

        // 读取第一个AMF包 - 通常是字符串，表示元数据类型
        quint8 type;
        stream >> type;

        if (type != AMF_STRING) {
            m_metadata_values.key = QString("error type: 0x%1").arg(type, 2, 16, QChar('0'));
            printLogWithPos(
                QtWarningMsg,
                stream,
                QString("tag[script] error[script tag should start with AMF_STRING] error-type[%1]").arg((int) type));
            return true;
        }

        m_metadata_values.key = parseAMFString(stream);

        // 读取第二个AMF包 - 通常是ECMA数组，包含实际元数据
        if (!stream.atEnd()) {
            stream >> m_metadata_values.type;
            if (m_metadata_values.type == AMF_ECMA_ARRAY || m_metadata_values.type == AMF_OBJECT) {
                parseAMFObjectOrArray(stream, m_metadata_values);
                m_metadata_values.offset = startPos;
                m_metadata_values.size = stream.device()->pos() - startPos;
            } else {
                printLogWithPos(
                    QtWarningMsg, stream, QString("tag[script] unknown-type[%1]").arg((int) m_metadata_values.type));
                return true;
            }
        }
    } catch (const exception& e) {
        printLogWithPos(QtWarningMsg, stream, QString("tag[script] error[%1]").arg(e.what()));
    } catch (...) {
        printLogWithPos(QtWarningMsg, stream, QString("tag[script] error[unknown]"));
    }
    return true;
}

// 将 MetadataItem::toTreeObj 移到 cpp 实现
TreeItem* MetadataItem::toTreeObj() {
    static auto size_to_string = [](PropertyItem& it) -> QString {
        if (auto pd = std::get_if<double>(&it.value))
            if (*pd > 0)
                return QString("%1 (array size)").arg(*pd, 0, 'g', 10);
            else
                return QString("(object)");
        return QString();
    };

    static auto number_to_string = [](PropertyItem& it) -> QString {
        if (auto pd = std::get_if<double>(&it.value))
            return QString("%1 (double)").arg(*pd, 0, 'g', 10);
        return QString();
    };

    static auto boolean_to_string = [](PropertyItem& it) -> QString {
        if (auto pb = std::get_if<bool>(&it.value))
            return *pb ? "true (bool)" : "false (bool)";
        return QString();
    };

    static auto string_to_string = [](PropertyItem& it) -> QString {
        if (auto ps = std::get_if<string>(&it.value))
            return QString("%1 (string)").arg(QString::fromStdString(*ps));
        return QString();
    };

    switch (type) {
    case AMF_NUMBER:
        return new TreeItem(make_shared<PropertyItem>(key, offset, size, value, number_to_string), nullptr);
    case AMF_BOOLEAN:
        return new TreeItem(make_shared<PropertyItem>(key, offset, size, value, boolean_to_string), nullptr);
    case AMF_STRING:
        return new TreeItem(make_shared<PropertyItem>(key, offset, size, value, string_to_string), nullptr);
    case AMF_OBJECT:
    case AMF_ECMA_ARRAY:
    case AMF_STRICT_ARRAY: {
        auto root = new TreeItem(make_shared<PropertyItem>(key, offset, size, value, size_to_string), nullptr);
        for (auto& obj : obj_value) {
            root->appendChild(obj.toTreeObj());
        }
        return root;
    }
    default:
        return new TreeItem(make_shared<PropertyItem>(key, offset, size, value), nullptr);
    }
}

// VideoTagInfo 实现
VideoTagInfo::VideoTagInfo(FLVTag* m_tag_ptr) : m_tag_ptr(m_tag_ptr) {
    m_tag_type.reset(new PropertyItem("tag_type", -11, 1, 0.0));
    m_codec.reset(new PropertyItem("codec", -11, 1, 0.0));
    m_detail_type.reset(new PropertyItem("detail_type", -12, 1, 0.0));
    m_cts.reset(new PropertyItem("cts", -13, 3, 0.0));

    m_tag_type->toStringFunc = [](PropertyItem& item) {
        if (auto pd = std::get_if<double>(&item.value))
            return QString("%1 (%2)").arg(getTagType(*pd)).arg(*pd);
        return QString();
    };
    m_codec->toStringFunc = [](PropertyItem& item) {
        if (auto pd = std::get_if<double>(&item.value))
            return QString("%1 (%2)").arg(getCodec(*pd)).arg(*pd);
        return QString();
    };
    m_detail_type->toStringFunc = [](PropertyItem& item) {
        if (auto pd = std::get_if<double>(&item.value))
            return QString("%1 (%2)").arg(getDetailType(*pd)).arg(*pd);
        return QString();
    };
}

TreeItem* VideoTagInfo::toTreeObj() {
    int tagSize = 0;
    if (m_tag_ptr && m_tag_ptr->m_tag_size) {
        tagSize = static_cast<int>(get<double>(m_tag_ptr->m_tag_size->value));
    }
    auto info_tree = new TreeItem(make_shared<PropertyItem>("video_info", -11, tagSize, std::string()), nullptr);
    info_tree->appendChild(new TreeItem(m_tag_type, info_tree));
    info_tree->appendChild(new TreeItem(m_codec, info_tree));
    info_tree->appendChild(new TreeItem(m_detail_type, info_tree));
    info_tree->appendChild(new TreeItem(m_cts, info_tree));
    return info_tree;
}

// AudioTagInfo 实现
AudioTagInfo::AudioTagInfo(FLVTag* m_tag_ptr) : m_tag_ptr(m_tag_ptr) {
    m_sound_format.reset(new PropertyItem("sound_format", -11, 1, 0.0));
    m_sound_rate.reset(new PropertyItem("sound_rate", -11, 1, 0.0));
    m_sound_size.reset(new PropertyItem("sound_size", -11, 1, 0.0));
    m_sound_type.reset(new PropertyItem("sound_type", -11, 1, 0.0));
    m_detail_type.reset(new PropertyItem("detail_type", -12, 1, 0.0));

    static const QMap<uint8_t, const char*> soundSizeMap = {{0, "8-bit samples"}, {1, "16-bit samples"}};
    static const QMap<uint8_t, const char*> soundTypeMap = {{0, "Mono sound"}, {1, "Stereo sound"}};
    static const QMap<uint8_t, const char*> audioDataType = {{0, "Sequence header"}, {1, "normal data"}};

    m_sound_format->toStringFunc = [](PropertyItem& item) {
        if (auto pd = std::get_if<double>(&item.value))
            return QString("%1 (%2)").arg(getSoundFormat(*pd)).arg(*pd);
        return QString();
    };
    m_sound_rate->toStringFunc = [](PropertyItem& item) {
        if (auto pd = std::get_if<double>(&item.value))
            return QString("%1 (%2)").arg(getSoundRate(*pd)).arg(*pd);
        return QString();
    };
    m_sound_size->toStringFunc = [](PropertyItem& item) {
        if (auto pd = std::get_if<double>(&item.value))
            return QString("%1 (%2)").arg(soundSizeMap.value(*pd, "unknown")).arg(*pd);
        return QString();
    };
    m_sound_type->toStringFunc = [](PropertyItem& item) {
        if (auto pd = std::get_if<double>(&item.value))
            return QString("%1 (%2)").arg(soundTypeMap.value(*pd, "unknown")).arg(*pd);
        return QString();
    };
    m_detail_type->toStringFunc = [](PropertyItem& item) {
        if (auto pd = std::get_if<double>(&item.value))
            return QString("%1 (%2)").arg(audioDataType.value(*pd, "unknown")).arg(*pd);
        return QString();
    };
}

TreeItem* AudioTagInfo::toTreeObj() {
    int tagSize = 0;
    if (m_tag_ptr && m_tag_ptr->m_tag_size) {
        tagSize = static_cast<int>(get<double>(m_tag_ptr->m_tag_size->value));
    }
    auto info_tree = new TreeItem(make_shared<PropertyItem>("audio_info", -11, tagSize, std::string()), nullptr);
    info_tree->appendChild(new TreeItem(m_sound_format, info_tree));
    info_tree->appendChild(new TreeItem(m_sound_rate, info_tree));
    info_tree->appendChild(new TreeItem(m_sound_size, info_tree));
    info_tree->appendChild(new TreeItem(m_sound_type, info_tree));
    info_tree->appendChild(new TreeItem(m_detail_type, info_tree));
    return info_tree;
}

// DataTagInfo::toTreeObj 实现
TreeItem* DataTagInfo::toTreeObj() {
    int tagSize = 0;
    if (m_tag_ptr && m_tag_ptr->m_tag_size) {
        tagSize = static_cast<int>(get<double>(m_tag_ptr->m_tag_size->value));
    }
    auto info_tree = new TreeItem(make_shared<PropertyItem>("data_info", -11, tagSize, std::string()), nullptr);
    // 遍历所有元数据字段并添加到树中
    info_tree->appendChild(m_metadata_values.toTreeObj());
    return info_tree;
}

bool FLVTag::readfromStream(QDataStream& stream) {
    uint8_t buffer[64] = {0};

    m_offset = stream.device()->pos();
    uint8_t tag_type = 0;
    stream >> tag_type;
    m_tag_type->value = (double) tag_type; // metadata,音视频

    stream.readRawData(reinterpret_cast<char*>(buffer), 3);
    m_tag_size->value = (double) bigend_ctou24(buffer);
    m_size = get<double>(m_tag_size->value) + 11 + 4;

    stream.readRawData(reinterpret_cast<char*>(buffer), 3);
    m_timestamp->value = (double) bigend_ctou24(buffer);
    stream.readRawData(reinterpret_cast<char*>(buffer), 1);
    m_timestamp->value = get<double>(m_timestamp->value) + (static_cast<uint32_t>(buffer[0]) << 24);

    stream.readRawData(reinterpret_cast<char*>(buffer), 3);
    m_stream_id->value = (double) bigend_ctou24(buffer);

    int type = static_cast<int>(get<double>(m_tag_type->value));
    printLogWithPos(QtDebugMsg, stream, QString("event[tag_read] type[%1]").arg(type));

    switch (type) {
    case TAG_TYPE_SCRIPT: {
        // 读取metadata
        metadata_info = make_unique<DataTagInfo>(this);
        // 使用AMF解析函数解析元数据
        metadata_info->ReadFromStream(stream);
    } break;
    case TAG_TYPE_AUDIO: {
        a_info = make_unique<AudioTagInfo>(this);

        // 读取音频帧头信息(1字节)
        stream.readRawData(reinterpret_cast<char*>(buffer), 1);
        a_info->m_sound_format->value = (double) ((buffer[0] & 0xF0) >> 4); // 高4位
        a_info->m_sound_rate->value = (double) ((buffer[0] & 0x0C) >> 2);   // 3-2位
        a_info->m_sound_size->value = (double) ((buffer[0] & 0x02) >> 1);   // 1位
        a_info->m_sound_type->value = (double) (buffer[0] & 0x01);          // 0位

        if (get<double>(a_info->m_sound_format->value) == AAC) {
            // AAC
            stream.readRawData(reinterpret_cast<char*>(buffer), 1);
            a_info->m_detail_type->value = (double) buffer[0];
        }
    } break;
    case TAG_TYPE_VIDEO: {
        v_info = make_unique<VideoTagInfo>(this);

        // 读取视频帧头信息(1字节)
        stream.readRawData(reinterpret_cast<char*>(buffer), 1);
        v_info->m_tag_type->value = (double) ((buffer[0] & 0xF0) >> 4); // 高4位
        v_info->m_codec->value = (double) (buffer[0] & 0x0F);           // 低4位

        // 如果是AVC(H.264)需要额外读取CTS
        if (get<double>(v_info->m_codec->value) == AVC || get<double>(v_info->m_codec->value) == HEVC ||
            get<double>(v_info->m_codec->value) == AV1 || get<double>(v_info->m_codec->value) == VVC) {
            stream.readRawData(reinterpret_cast<char*>(buffer), 1);
            v_info->m_detail_type->value = (double) buffer[0];

            stream.readRawData(reinterpret_cast<char*>(buffer), 3);
            v_info->m_cts->value = static_cast<double>(bigend_ctoi24(buffer));
        }
    } break;
    default:
        break;
    }

    // 读取previous_tag_size
    int64_t offset_in_tag = 11 + get<double>(m_tag_size->value);
    m_previous_tag_size->offset = -offset_in_tag;
    if (stream.device()->pos() > m_offset + offset_in_tag) {
        printLogWithPos(QtWarningMsg, stream, QString("event[tag_content_error] reason[exceed tag size]"));
        return false;
    }

    stream.device()->seek(m_offset + offset_in_tag);
    stream.readRawData(reinterpret_cast<char*>(buffer), 4);
    m_previous_tag_size->value = (double) bigend_ctou32(buffer);

    // 读取帧的二进制数据
    m_bin_data.reset(new uchar[offset_in_tag + 4]);
    stream.device()->seek(m_offset);
    stream.readRawData((char*) m_bin_data.get(), offset_in_tag + 4);

    if (stream.status() != QDataStream::Ok) {
        printLogWithPos(QtInfoMsg, stream, QString("event[finished]"));
        return false;
    }

    return true;
}

// FLVTag::getTreeInfo 实现
shared_ptr<TreeItem>& FLVTag::getTreeInfo() {
    if (m_info_tree) {
        return m_info_tree;
    }

    m_info_tree.reset(new TreeItem(make_shared<PropertyItem>("tag_info", 0, 1, std::string()), nullptr));
    m_info_tree->appendChild(new TreeItem(m_tag_type, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_tag_size, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_timestamp, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_stream_id, m_info_tree.get()));

    if (get<double>(m_tag_type->value) == TAG_TYPE_SCRIPT && metadata_info) {
        m_info_tree->appendChild(metadata_info->toTreeObj());
    } else if (get<double>(m_tag_type->value) == TAG_TYPE_VIDEO && v_info) {
        m_info_tree->appendChild(v_info->toTreeObj());
    } else if (get<double>(m_tag_type->value) == TAG_TYPE_AUDIO && a_info) {
        m_info_tree->appendChild(a_info->toTreeObj());
    } else {
        auto unknown = new TreeItem(make_shared<PropertyItem>("unknown", 0, 1, 0.0), m_info_tree.get());
        m_info_tree->appendChild(unknown);
    }

    m_info_tree->appendChild(new TreeItem(m_previous_tag_size, m_info_tree.get()));
    return m_info_tree;
}

bool FLVHeader::readfromStream(QDataStream& stream) {
    m_offset = 0;
    m_size = 13;

    uint8_t buffer[16] = {0};
    stream.readRawData(reinterpret_cast<char*>(buffer), 13);

    if (stream.status() != QDataStream::Ok) {
        printLogWithPos(QtInfoMsg, stream, QString("event[finished]"));
        return false;
    }

    if (0 != memcmp(buffer, "FLV", 3)) {
        printLogWithPos(QtWarningMsg, stream, QString("event[flv_header_error]"));
        return false;
    }

    m_signature->value = string((const char*) (buffer), 3);
    m_version->value = (double) buffer[3];
    m_type_flags->value = (double) buffer[4];
    m_data_offset->value = (double) bigend_ctou32(buffer + 5);
    m_previous_tag_size->value = (double) bigend_ctou32(buffer + 9);

    // 读取帧的二进制数据
    m_bin_data.reset(new uchar[13]);
    stream.device()->seek(0);
    stream.readRawData((char*) m_bin_data.get(), 13);

    return true;
}

shared_ptr<TreeItem>& FLVHeader::getTreeInfo() {
    if (m_info_tree) {
        return m_info_tree;
    }

    m_info_tree.reset(new TreeItem(make_shared<PropertyItem>("flv_header", 0, 9, std::string()), nullptr));
    m_info_tree->appendChild(new TreeItem(m_signature, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_version, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_type_flags, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_data_offset, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_previous_tag_size, m_info_tree.get()));

    return m_info_tree;
}
