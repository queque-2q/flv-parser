#ifndef FRAMEINFO_H
#define FRAMEINFO_H

#include <QPair>
#include <QList>
#include <QMap>

using namespace std;

enum TAG_TYPE
{
    TAG_TYPE_AUDIO = 8,
    TAG_TYPE_VIDEO = 9,
    TAG_TYPE_METADATA = 18
};

/**
 * @class TreeItem
 * @brief 和树型QAbstractItemModel绑定的通用类
 */
struct TreeItem
{
    TreeItem(
        const QPair<QString, QString> &data, TreeItem *parent = nullptr)
        : itemData(data), parentItem(parent)
    {
    }

    ~TreeItem()
    {
        for (auto child : childItems)
        {
            delete child;
        }
    }

    void appendChild(
        TreeItem *child)
    {
        childItems.append(child);
    }

    TreeItem *child(
        int row)
    {
        return childItems.value(row);
    }

    int childCount() const { return childItems.count(); }

    int columnCount() const { return 2; }

    int row() const
    {
        if (parentItem)
            return parentItem->childItems.indexOf(const_cast<TreeItem *>(this));
        return 0;
    }

    QPair<QString, QString> itemData;
    TreeItem *parentItem;
    QList<TreeItem *> childItems;
};


// AMF数据类型
enum AMF_DATA_TYPE {
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
QString parseAMFString(QDataStream &stream);

// 解析AMF数字
double parseAMFNumber(QDataStream &stream);

// 解析AMF布尔值
bool parseAMFBoolean(QDataStream &stream);

/**
 * @class DataFrameInfo
 * @brief 元数据帧详细字段
 */
struct DataFrameInfo
{
    QString m_metadata_type;
    QMap<QString, QString> m_metadata_values;

    void ReadFromStream(QDataStream &stream);
    void parseAMFObject(QDataStream &stream, bool isECMAArray = false);

    TreeItem *toTreeObj()
    {
        TreeItem *root = new TreeItem(qMakePair("frameInfo", ""), nullptr);

        root->appendChild(
            new TreeItem(qMakePair("metadata_type", m_metadata_type), root));

        // 创建元数据值的子节点
        TreeItem *valuesNode = new TreeItem(qMakePair("metadata_values", ""), root);
        root->appendChild(valuesNode);

        // 遍历所有元数据字段并添加到树中
        QMapIterator<QString, QString> it(m_metadata_values);
        while (it.hasNext()) {
            it.next();
            valuesNode->appendChild(
                new TreeItem(qMakePair(it.key(), it.value()), valuesNode));
        }

        return root;
    }
};

/**
 * @class VideoFrameInfo
 * @brief 视频帧详细字段
 */
struct VideoFrameInfo
{
    uint8_t m_frame_type = 0;
    uint8_t m_codec = 0;
    uint8_t m_detail_type = 0;
    int m_cts = 0;

    TreeItem *toTreeObj()
    {
        TreeItem *root = new TreeItem(qMakePair("frameInfo", ""), nullptr);

        root->appendChild(
            new TreeItem(qMakePair("frame_type", QString::fromStdString(to_string(m_frame_type))),
                         root));
        root->appendChild(
            new TreeItem(qMakePair("codec", QString::fromStdString(to_string(m_codec))), root));
        root->appendChild(
            new TreeItem(qMakePair("detail_type", QString::fromStdString(to_string(m_detail_type))),
                         root));
        root->appendChild(
            new TreeItem(qMakePair("cts", QString::fromStdString(to_string(m_cts))), root));

        return root;
    }
};

/**
 * @class AudioFrameInfo
 * @brief 音频帧详细字段
 */
struct AudioFrameInfo
{
    uint8_t m_sound_format = 0;
    uint8_t m_sound_rate = 0;
    uint8_t m_sound_size = 0;
    uint8_t m_sound_type = 0;
    int m_detail_type = 0;

    TreeItem *toTreeObj()
    {
        TreeItem *root = new TreeItem(qMakePair("frameInfo", ""), nullptr);

        root->appendChild(new TreeItem(qMakePair("sound_format",
                                                 QString::fromStdString(to_string(m_sound_format))),
                                       root));
        root->appendChild(
            new TreeItem(qMakePair("sound_rate", QString::fromStdString(to_string(m_sound_rate))),
                         root));
        root->appendChild(
            new TreeItem(qMakePair("m_sound_size", QString::fromStdString(to_string(m_sound_size))),
                         root));

        root->appendChild(
            new TreeItem(qMakePair("sound_type", QString::fromStdString(to_string(m_sound_type))),
                         root));
        root->appendChild(
            new TreeItem(qMakePair("detail_type", QString::fromStdString(to_string(m_detail_type))),
                         root));
        return root;
    }
};

/**
 * @class BinaryData
 * @brief 二进制结构体
 */
struct BinaryData
{
    virtual ~BinaryData() {}

    shared_ptr<uchar[]> m_bin_data;
    uint64_t m_offset = 0;
    uint32_t m_size = 0;
};

/**
 * @class ModelFLVFrame
 * @brief flv帧信息，包括flv帧头和帧数据信息
 */
struct ModelFLVFrame : public BinaryData
{
public:
    uint8_t m_tag_type = 0;
    uint32_t m_frame_size = 0;
    uint32_t m_timestamp = 0;
    uint32_t m_stream_id = 0;

    // parsed info
    shared_ptr<DataFrameInfo> metadata_info;
    shared_ptr<VideoFrameInfo> v_info;
    shared_ptr<AudioFrameInfo> a_info;
};


#endif // MODELFRAMEINFO_H
