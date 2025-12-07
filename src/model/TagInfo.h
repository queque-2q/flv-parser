// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QSharedPointer>
#include <QString>
#include <QVector>
#include <functional>

using namespace std;

enum TAG_TYPE {
    TAG_TYPE_AUDIO = 8,
    TAG_TYPE_VIDEO = 9,
    TAG_TYPE_SCRIPT = 18
};

enum FLV_AUDIO_CODEC {
    AAC = 10
};

enum FLV_VIDEO_CODEC {
    AVC = 7,
    HEVC = 12,
    AV1 = 13,
    VVC = 14
};

/**
 * @class PropertyItem
 * @brief 字段类，用于树型结构展示
 */
using property_variant = std::variant<double, bool, string>;
struct PropertyItem {
    QString name;
    int64_t offset = 0; // 偏移量
    uint32_t size = 0;  // 大小
    property_variant value;

    function<QString(PropertyItem&)> toStringFunc = [](PropertyItem& it) -> QString {
        if (auto pd = std::get_if<double>(&it.value))
            return QString("%1").arg(*pd, 0, 'g', 10);
        if (auto pb = std::get_if<bool>(&it.value))
            return *pb ? "true" : "false";
        if (auto ps = std::get_if<std::string>(&it.value))
            return QString("%1").arg(QString::fromStdString(*ps));
        return QString();
    };

    PropertyItem(const QString& n, int64_t off, uint32_t s, const property_variant& v)
        : name(n), offset(off), size(s), value(v) {
    }

    PropertyItem(const QString& n,
                 int64_t off,
                 uint32_t s,
                 const property_variant& v,
                 function<QString(PropertyItem&)> func)
        : name(n), offset(off), size(s), value(v), toStringFunc(func) {
    }
};

/**
 * @class TreeItem
 * @brief 和树型QAbstractItemModel绑定的通用类
 */
struct TreeItem {
    TreeItem(const shared_ptr<PropertyItem>& data, TreeItem* parent = nullptr) : data(data), parentItem(parent) {
    }

    ~TreeItem() {
        for (auto& child : childItems) {
            delete child;
        }
    }

    void appendChild(TreeItem* child) {
        childItems.append(child);

        if (child->parentItem != this) {
            child->parentItem = this;
        }
    }

    TreeItem* child(int row) {
        return childItems.value(row);
    }

    int childCount() const {
        return childItems.count();
    }

    int columnCount() const {
        return 2;
    }

    int row() const {
        if (parentItem)
            return parentItem->childItems.indexOf(this);
        return 0;
    }

    TreeItem* parentItem;
    QList<TreeItem*> childItems; // owner

    shared_ptr<PropertyItem> data;
};

// AMF数据类型
enum AMF_DATA_TYPE : char {
    AMF_NUMBER = 0x00,
    AMF_BOOLEAN = 0x01,
    AMF_STRING = 0x02,
    AMF_OBJECT = 0x03,
    AMF_MOVIECLIP = 0x04,
    AMF_NULL = 0x05,
    AMF_UNDEFINED = 0x06,
    AMF_REFERENCE = 0x07,
    AMF_ECMA_ARRAY = 0x08,
    AMF_OBJECT_END = 0x09,
    AMF_STRICT_ARRAY = 0x0A,
    AMF_DATE = 0x0B,
    AMF_LONG_STRING = 0x0C,
    AMF_UNSUPPORTED = 0x0D,
    AMF_RECORDSET = 0x0E,
    AMF_XML_DOCUMENT = 0x0F,
    AMF_TYPED_OBJECT = 0x10,
    AMF_AVMPLUS_OBJECT = 0x11
};

// 解析AMF字符串
QString parseAMFString(QDataStream& stream);

// 解析AMF数字
double parseAMFNumber(QDataStream& stream);

// 解析AMF布尔值
bool parseAMFBoolean(QDataStream& stream);

/**
 * @class DataTagInfo
 * @brief 元数据帧详细字段
 */
struct MetadataItem {
    char type = 0;
    QString key;

    property_variant value; // 通用值
    vector<MetadataItem> obj_value;

    int64_t offset = 0; // 偏移量
    uint32_t size = 0;  // 大小

    MetadataItem() = default;

    MetadataItem(char t, const QString& k, int64_t off = 0, uint32_t s = 0) : type(t), key(k), offset(off), size(s) {
    }
    TreeItem* toTreeObj();
};

/**
 * @class FLVTag
 * @brief flv帧信息，包括flv帧头和帧数据信息
 */
struct FLVTag;

/**
 * @class DataTagInfo
 * @brief 元数据帧详细字段
 */
struct DataTagInfo {
    MetadataItem m_metadata_values;
    FLVTag* m_tag_ptr = nullptr; // 指向所属的FLVTag

    DataTagInfo(FLVTag* m_tag_ptr) : m_tag_ptr(m_tag_ptr) {
    }

    bool ReadFromStream(QDataStream& stream);
    void parseAMFObjectOrArray(QDataStream& stream, MetadataItem& item);
    TreeItem* toTreeObj();
};

inline const char* getTagType(uint8_t tag_type) {
    switch (tag_type) {
    case 1:
        return "keytag";
    case 2:
        return "inter tag";
    case 3:
        return "disposable inter tag";
    case 4:
        return "generated keytag";
    case 5:
        return "video info/command tag";
    default:
        return "unknown tag type";
    }
}

inline const char* getCodec(uint8_t codec) {
    switch (codec) {
    case 2:
        return "Sorenson H.263";
    case 3:
        return "Screen video";
    case 4:
        return "On2 VP6";
    case 5:
        return "On2 VP6 with alpha channel";
    case 6:
        return "Screen video version 2";
    case 7:
        return "AVC";
    case 12:
        return "HEVC";
    case 13:
        return "AV1";
    case 14:
        return "VVC";
    default:
        return "unknown codec";
    }
}
inline const char* getDetailType(uint8_t detail_type) {
    switch (detail_type) {
    case 0:
        return "sequence header";
    case 1:
        return "normal NALU";
    case 2:
        return "end of sequence";
    default:
        return "unknown detail type";
    }
}
inline const char* getSoundFormat(uint8_t sound_format) {
    switch (sound_format) {
    case 0:
        return "Linear PCM, platform endian";
    case 1:
        return "ADPCM";
    case 2:
        return "MP3";
    case 3:
        return "Linear PCM, little endian";
    case 4:
        return "Nellymoser 16-kHz mono";
    case 5:
        return "Nellymoser 8-kHz mono";
    case 6:
        return "Nellymoser";
    case 7:
        return "G.711 A-law";
    case 8:
        return "G.711 mu-law";
    case 9:
        return "reserved";
    case 10:
        return "AAC";
    case 11:
        return "Speex";
    case 14:
        return "MP3 8-Khz";
    case 15:
        return "Device-specific sound";
    default:
        return "unknown sound format";
    }
}
inline const char* getSoundRate(uint8_t sound_rate) {
    switch (sound_rate) {
    case 0:
        return "5.5-kHz";
    case 1:
        return "11-kHz";
    case 2:
        return "22-kHz";
    case 3:
        return "44-kHz";
    default:
        return "unknown sound rate";
    }
}

/**
 * @class VideoTagInfo
 * @brief 视频帧详细字段
 */
struct VideoTagInfo {
  public:
    // 字段
    shared_ptr<PropertyItem> m_tag_type;
    shared_ptr<PropertyItem> m_codec;
    shared_ptr<PropertyItem> m_detail_type;
    shared_ptr<PropertyItem> m_cts;

    // 指向所属的FLVTag，便于修改
    FLVTag* m_tag_ptr = nullptr;

    VideoTagInfo(FLVTag* m_tag_ptr);
    TreeItem* toTreeObj();
};

/**
 * @class AudioTagInfo
 * @brief 音频帧详细字段
 */
struct AudioTagInfo {
  public:
    // 字段
    shared_ptr<PropertyItem> m_sound_format;
    shared_ptr<PropertyItem> m_sound_rate;
    shared_ptr<PropertyItem> m_sound_size;
    shared_ptr<PropertyItem> m_sound_type;
    shared_ptr<PropertyItem> m_detail_type;

    // 指向所属的FLVTag，便于修改
    FLVTag* m_tag_ptr = nullptr;

    AudioTagInfo(FLVTag* m_tag_ptr);
    TreeItem* toTreeObj();
};

/**
 * @class BinaryData
 * @brief 二进制结构体
 */
struct BinaryData {
    virtual ~BinaryData() {
    }

    shared_ptr<uchar[]> m_bin_data;
    uint64_t m_offset = 0;
    uint32_t m_size = 0;

};

/**
 * @class FLVTag
 * @brief flv帧信息，包括flv帧头和帧数据信息
 */
struct FLVTag : public BinaryData {
  public:
    shared_ptr<PropertyItem> m_tag_type;
    shared_ptr<PropertyItem> m_tag_size;
    shared_ptr<PropertyItem> m_timestamp;
    shared_ptr<PropertyItem> m_stream_id;
    shared_ptr<PropertyItem> m_previous_tag_size;

    // 帧信息
    unique_ptr<DataTagInfo> metadata_info;
    unique_ptr<VideoTagInfo> v_info;
    unique_ptr<AudioTagInfo> a_info;

    // 树状信息指针
    shared_ptr<TreeItem> m_info_tree;

    FLVTag() {
        m_tag_type.reset(new PropertyItem("tag_type", 0, 1, 0.0));
        m_tag_size.reset(new PropertyItem("tag_size", -1, 3, 0.0));
        m_timestamp.reset(new PropertyItem("timestamp", -4, 4, 0.0));
        m_stream_id.reset(new PropertyItem("stream_id", -8, 3, 0.0));
        m_previous_tag_size.reset(new PropertyItem("previous_tag_size", 0, 4, 0.0));

        static const QMap<uint8_t, const char*> tagTypeMap = {
            {TAG_TYPE_AUDIO, "audio"}, {TAG_TYPE_VIDEO, "video"}, {TAG_TYPE_SCRIPT, "script"}};

        m_tag_type->toStringFunc = [](PropertyItem& item) {
            if (auto pd = std::get_if<double>(&item.value))
                return QString("%1 (%2)").arg(tagTypeMap.value(*pd, "unknown")).arg(*pd);
            return QString();
        };
    }

    bool readfromStream(QDataStream& stream);
    shared_ptr<TreeItem>& getTreeInfo();
};

/**
 * @class FLVHeader
 * @brief flv文件头结构体
 */
struct FLVHeader : public BinaryData {
    shared_ptr<PropertyItem> m_signature;
    shared_ptr<PropertyItem> m_version;
    shared_ptr<PropertyItem> m_type_flags;
    shared_ptr<PropertyItem> m_data_offset;
    shared_ptr<PropertyItem> m_previous_tag_size;

    // 树状信息指针
    shared_ptr<TreeItem> m_info_tree;

    FLVHeader() {
        m_signature.reset(new PropertyItem("signature", 0, 3, string()));
        m_version.reset(new PropertyItem("version", -3, 1, 0.0));
        m_type_flags.reset(new PropertyItem("type_flags", -4, 1, 0.0));
        m_data_offset.reset(new PropertyItem("data_offset", -5, 4, 0.0));
        m_previous_tag_size.reset(new PropertyItem("previous_tag_size", -9, 4, 0.0));

        m_type_flags->toStringFunc = [](PropertyItem& item) {
            if (auto ps = std::get_if<double>(&item.value)) {
                int flags = static_cast<int>(*ps);
                if ((flags & 0x05) == 0x05)
                    return QString("has audio and video (%1)").arg(*ps);
                else if ((flags & 0x04) == 0x04)
                    return QString("has video (%1)").arg(*ps);
                else if (flags & 0x01)
                    return QString("has audio (%1)").arg(*ps);
                else
                    return QString("%1").arg(*ps);
            }
            return QString();
        };
    }

    bool readfromStream(QDataStream& stream);
    shared_ptr<TreeItem>& getTreeInfo();
};
