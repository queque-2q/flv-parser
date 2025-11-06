// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "frameinfo.h"
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
void DataFrameInfo::parseAMFObject(QDataStream& stream, MetadataItem& item, bool isECMAArray) {
    // 如果是ECMA数组，跳过数组长度字段
    quint32 arrayLen = UINT32_MAX;
    if (isECMAArray) {
        stream >> arrayLen;
    }

    while (arrayLen > 0) {
        // 读取属性名
        QString propertyName = parseAMFString(stream);

        // 检查是否到达对象结束标记
        if (propertyName.isEmpty()) {
            quint8 endMarker;
            stream >> endMarker;
            if (endMarker == AMF_OBJECT_END)
                break;
            continue;
        }

        arrayLen--;

        // 读取属性值类型
        stream >> item.type;

        // 根据类型解析值
        switch (item.type) {
        case AMF_NUMBER: {
            double value = parseAMFNumber(stream);

            item.obj_value.emplace_back(MetadataItem{item.type, propertyName});
            item.obj_value.back().value = value;
            break;
        }
        case AMF_BOOLEAN: {
            bool value = parseAMFBoolean(stream);
            item.obj_value.emplace_back(MetadataItem{item.type, propertyName});
            item.obj_value.back().value = value;
            break;
        }
        case AMF_STRING: {
            QString value = parseAMFString(stream);
            item.obj_value.emplace_back(MetadataItem{item.type, propertyName});
            item.obj_value.back().value = value.toStdString();
            break;
        }
        case AMF_OBJECT: {
            // 递归解析对象
            MetadataItem subItem(item.type, propertyName);
            item.obj_value.emplace_back(move(subItem));
            parseAMFObject(stream, item.obj_value.back());
            break;
        }
        default: {
            // 对于其他复杂类型，简单跳过
            item.obj_value.emplace_back(MetadataItem{item.type, propertyName});
            item.obj_value.back().value = "null/undefined";
            throw runtime_error("未知类型: " + to_string(item.type));
        }
        }
    }
}

// 解析AMF数据
void DataFrameInfo::ReadFromStream(QDataStream& stream) {
    try {
        // 读取第一个AMF包 - 通常是字符串，表示元数据类型
        quint8 type;
        stream >> type;

        if (type != AMF_STRING) {
            m_metadata_values.key = QString("未知类型: 0x%1").arg(type, 2, 16, QChar('0'));
            qCDebug(runLog) << "元数据类型不是字符串，而是:" << type;
            return;
        }

        m_metadata_values.key = parseAMFString(stream);

        // 读取第二个AMF包 - 通常是ECMA数组，包含实际元数据
        if (!stream.atEnd()) {
            stream >> type;
            if (type == AMF_ECMA_ARRAY) {
                parseAMFObject(stream, m_metadata_values, true);
            } else if (type == AMF_OBJECT) {
                parseAMFObject(stream, m_metadata_values);
            } else {
                qCDebug(runLog) << "元数据值不是ECMA数组或对象，而是:" << type;
                return;
            }
        }
    } catch (const exception& e) {
        qCDebug(runLog) << "解析AMF数据时发生异常:" << e.what();
    } catch (...) {
        qCDebug(runLog) << "解析AMF数据时发生未知异常";
    }
}

// 将 MetadataItem::toTreeObj 移到 cpp 实现
TreeItem* MetadataItem::toTreeObj() {
    static auto number_to_string = [](PropertyItem& it) -> QString {
        if (auto pd = std::get_if<double>(&it.value))
            return QString("%1 (double)").arg(*pd);
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
        return new TreeItem(make_shared<PropertyItem>(key, 11, 1, value, number_to_string), nullptr);
    case AMF_BOOLEAN:
        return new TreeItem(make_shared<PropertyItem>(key, 11, 1, value, boolean_to_string), nullptr);
    case AMF_STRING:
        return new TreeItem(make_shared<PropertyItem>(key, 11, 1, value, string_to_string), nullptr);
    case AMF_OBJECT: {
        auto root = new TreeItem(make_shared<PropertyItem>(key, 11, 1, std::string()), nullptr);
        for (auto& obj : obj_value) {
            root->appendChild(obj.toTreeObj());
        }
        return root;
    }
    default:
        return new TreeItem(make_shared<PropertyItem>(key, 11, 1, value), nullptr);
    }
}

// VideoFrameInfo 实现
VideoFrameInfo::VideoFrameInfo(FLVFrame* m_frame_ptr) : m_frame_ptr(m_frame_ptr) {
    m_frame_type.reset(new PropertyItem("frame_type", 11, 1, 0.0));
    m_codec.reset(new PropertyItem("codec", 11, 1, 0.0));
    m_detail_type.reset(new PropertyItem("detail_type", 12, 1, 0.0));
    m_cts.reset(new PropertyItem("cts", 13, 3, 0.0));

    m_frame_type->toStringFunc = [](PropertyItem& item) {
        if (auto pd = std::get_if<double>(&item.value))
            return QString("%1 (%2)").arg(getFrameType(*pd)).arg(*pd);
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

TreeItem* VideoFrameInfo::toTreeObj() {
    int frameSize = 0;
    if (m_frame_ptr && m_frame_ptr->m_tag_size) {
        frameSize = static_cast<int>(get<double>(m_frame_ptr->m_tag_size->value));
    }
    auto info_tree = new TreeItem(make_shared<PropertyItem>("video_info", 11, frameSize, std::string()), nullptr);
    info_tree->appendChild(new TreeItem(m_frame_type, info_tree));
    info_tree->appendChild(new TreeItem(m_codec, info_tree));
    info_tree->appendChild(new TreeItem(m_detail_type, info_tree));
    info_tree->appendChild(new TreeItem(m_cts, info_tree));
    return info_tree;
}

// AudioFrameInfo 实现
AudioFrameInfo::AudioFrameInfo(FLVFrame* m_frame_ptr) : m_frame_ptr(m_frame_ptr) {
    m_sound_format.reset(new PropertyItem("sound_format", 11, 1, 0.0));
    m_sound_rate.reset(new PropertyItem("sound_rate", 11, 1, 0.0));
    m_sound_size.reset(new PropertyItem("sound_size", 11, 1, 0.0));
    m_sound_type.reset(new PropertyItem("sound_type", 11, 1, 0.0));
    m_detail_type.reset(new PropertyItem("detail_type", 12, 1, 0.0));

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

TreeItem* AudioFrameInfo::toTreeObj() {
    int frameSize = 0;
    if (m_frame_ptr && m_frame_ptr->m_tag_size) {
        frameSize = static_cast<int>(get<double>(m_frame_ptr->m_tag_size->value));
    }
    auto info_tree = new TreeItem(make_shared<PropertyItem>("audio_info", 11, frameSize, std::string()), nullptr);
    info_tree->appendChild(new TreeItem(m_sound_format, info_tree));
    info_tree->appendChild(new TreeItem(m_sound_rate, info_tree));
    info_tree->appendChild(new TreeItem(m_sound_size, info_tree));
    info_tree->appendChild(new TreeItem(m_sound_type, info_tree));
    info_tree->appendChild(new TreeItem(m_detail_type, info_tree));
    return info_tree;
}

// DataFrameInfo::toTreeObj 实现
TreeItem* DataFrameInfo::toTreeObj() {
    int frameSize = 0;
    if (m_frame_ptr && m_frame_ptr->m_tag_size) {
        frameSize = static_cast<int>(get<double>(m_frame_ptr->m_tag_size->value));
    }
    auto info_tree = new TreeItem(make_shared<PropertyItem>("data_info", 11, frameSize, std::string()), nullptr);
    // 遍历所有元数据字段并添加到树中
    info_tree->appendChild(m_metadata_values.toTreeObj());
    return info_tree;
}

// FLVFrame::getTreeInfo 实现
shared_ptr<TreeItem>& FLVFrame::getTreeInfo() {
    if (m_info_tree) {
        return m_info_tree;
    }

    m_info_tree.reset(new TreeItem(make_shared<PropertyItem>("frame_info", 0, 1, std::string()), nullptr));
    m_info_tree->appendChild(new TreeItem(m_tag_type, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_tag_size, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_timestamp, m_info_tree.get()));
    m_info_tree->appendChild(new TreeItem(m_stream_id, m_info_tree.get()));

    if (get<double>(m_tag_type->value) == TAG_TYPE_METADATA && metadata_info) {
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
