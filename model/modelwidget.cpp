/**
 * @file modelframeinfo.cpp
 * @brief FLV帧信息模型类的实现文件
 * @details 实现了以下几个主要模型类：
 *          - ModelFLVStream: FLV流数据的基础类
 *          - ModelFrameList: 帧列表展示模型，用于TableView显示
 *          - ModelFrameInfoTree: 帧详细信息树形模型，用于TreeView显示
 *          - ModelFrameBinary: 帧二进制数据模型，用于展示原始数据
 * @author Your Name
 * @date 2024-03-11
 * @version 1.0
 */

#pragma execution_character_set("utf-8")

#include "modelwidget.h"
#include <QMessageBox>
#include <QSize>
#include <QBuffer>
#include "utils.h"
#include <vector>
#include <QDebug>


int ModelFrameList::rowCount(const QModelIndex &parent) const
{
    return m_frameList.size();
}
int ModelFrameList::columnCount(const QModelIndex &parent) const
{
    return ModelFrameList::column_size;
}

QVariant ModelFrameList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_frameList.size() || index.column() >= ModelFrameList::column_size || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    int row = index.row();
    int column = index.column();

    switch (column)
    {
    case 0:
        return QString("0x%1").arg(
            QString::number(uint(m_frameList[row]->m_offset), 16).rightJustified(8, '0'));
    case 1:
        return QString("%1").arg(m_frameList[row]->m_tag_type);
    case 2:
        return QString("%1").arg(m_frameList[row]->m_size);
    case 3:
        return QString("%1").arg(m_frameList[row]->m_timestamp);
    case 4:
        if (m_frameList[row]->v_info)
            return QString("%1").arg(m_frameList[row]->v_info->m_detail_type);
        if (m_frameList[row]->a_info)
            return QString("%1").arg(m_frameList[row]->a_info->m_detail_type);
    default:
        return QVariant();
    }

    return QVariant();
}

QVariant ModelFrameList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        // 根据列索引返回相应的表头数据
        const char *header[] = {"偏移地址", "类型", "长度", "时间戳", "详细信息"};
        if (section < ModelFrameList::column_size)
            return QString(header[section]);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int ModelFrameList::readFromFile(
    QFile &file)
{
    QDataStream stream(&file);
    vector<shared_ptr<ModelFLVFrame>> frame_vec;

    // 解析帧

    // flv头
    QByteArray buffer(64, 0);
    if (3 != stream.readRawData(buffer.data(), 3))
    {
        throw QString("文件格式错误");
    }

    if (0 != memcmp(buffer.data(), "FLV", 3))
    {
        throw QString("文件格式错误");
    }

    stream.skipRawData(10);

    uint32_t previous_tag_size = 0;
    int64_t len = 0;
    while (!stream.atEnd())
    {
        shared_ptr<ModelFLVFrame> frame_info = make_shared<ModelFLVFrame>();

        frame_info->m_offset = file.pos();
        stream >> frame_info->m_tag_type; // metadata,音视频

        if (3 != stream.readRawData(buffer.data(), 3))
            throw QString("文件结束");
        frame_info->m_frame_size = GetUint24(reinterpret_cast<unsigned char *>(buffer.data()));
        frame_info->m_size = frame_info->m_frame_size + 11;

        if (3 != stream.readRawData(buffer.data(), 3))
            throw QString("文件结束");
        frame_info->m_timestamp = GetUint24(reinterpret_cast<unsigned char *>(buffer.data()));
        if (1 != stream.readRawData(buffer.data(), 1))
            throw QString("文件结束");
        frame_info->m_timestamp += *reinterpret_cast<unsigned char *>(buffer.data()) << 24;

        if (3 != stream.readRawData(buffer.data(), 3))
            throw QString("文件结束");
        frame_info->m_stream_id = GetUint24(reinterpret_cast<unsigned char *>(buffer.data()));

        switch (frame_info->m_tag_type)
        {
        case TAG_TYPE_METADATA:
            {
            // 读取metadata
            frame_info->metadata_info = make_shared<DataFrameInfo>();
            // 使用AMF解析函数解析元数据
            frame_info->metadata_info->ReadFromStream(stream);
            }
            break;
        case TAG_TYPE_AUDIO:
            {
            frame_info->a_info = make_shared<AudioFrameInfo>();
            /*uint8_t m_sound_format;
                uint8_t m_sound_rate;
                uint8_t m_sound_size;
                uint8_t m_sound_type;
                int m_detail_type;*/

            // 读取音频帧头信息(1字节)
            if (1 != stream.readRawData(buffer.data(), 1))
                throw QString("文件结束");
            frame_info->a_info->m_sound_format = (buffer.data()[0] & 0xF0) >> 4; // 高4位
            frame_info->a_info->m_sound_rate = (buffer.data()[0] & 0x0C) >> 2;   // 3-2位
            frame_info->a_info->m_sound_size = (buffer.data()[0] & 0x02) >> 1;   // 1位
            frame_info->a_info->m_sound_type = buffer.data()[0] & 0x01;          // 0位

            if (frame_info->a_info->m_sound_format == 10)
            {
                // AAC
                if (1 != stream.readRawData(buffer.data(), 1))
                    throw QString("文件结束");
                frame_info->a_info->m_detail_type = buffer.data()[0];
            }
            }
            break;
        case TAG_TYPE_VIDEO:
            {
            frame_info->v_info = make_shared<VideoFrameInfo>();
            /*uint8_t m_frame_type;
                uint8_t m_codec;
                int m_cts;
                int m_detail_type;*/

            // 读取视频帧头信息(1字节)
            if (1 != stream.readRawData(buffer.data(), 1))
                throw QString("文件结束");
            frame_info->v_info->m_frame_type = (buffer.data()[0] & 0xF0) >> 4; // 高4位
            frame_info->v_info->m_codec = buffer.data()[0] & 0x0F;             // 低4位

            // 如果是AVC(H.264)需要额外读取CTS
            if (frame_info->v_info->m_codec == 7 || frame_info->v_info->m_codec == 12 || frame_info->v_info->m_codec == 13 || frame_info->v_info->m_codec == 14)
            {
                if (1 != stream.readRawData(buffer.data(), 1))
                    throw QString("文件结束");
                frame_info->v_info->m_detail_type = buffer.data()[0];

                if (3 != stream.readRawData(buffer.data(), 3))
                    throw QString("文件结束");
                frame_info->v_info->m_cts = GetInt24(reinterpret_cast<unsigned char *>(buffer.data()));
            }
            }
            break;
        default:
            break;
        }

        frame_info->m_bin_data.reset(new uchar[11 + frame_info->m_frame_size + 4]);
        file.seek(frame_info->m_offset);
        stream.readRawData((char *)frame_info->m_bin_data.get(), 11 + frame_info->m_frame_size + 4);

        file.seek(file.pos() - 4);
        if (4 != stream.readRawData(buffer.data(), 4))
            throw QString("文件结束");
        previous_tag_size = GetUint32(reinterpret_cast<unsigned char *>(buffer.data()));

        frame_vec.emplace_back(frame_info);
    }

    m_frameList.swap(frame_vec);
    return 0;
}

/**
 @class ModelFrameInfoTree
*/

QVariant ModelFrameInfoTree::data(
    const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::SizeHintRole)
        return QSize(200, 30);

    TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
    if (role == Qt::DisplayRole)
    {
        if (index.column() == 0)
            return item->itemData.first; // 返回数据（QString）
        if (index.column() == 1)
            return item->itemData.second; // 返回数据（QString）
    }
    return QVariant();
}

QModelIndex ModelFrameInfoTree::parent(
    const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem *>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem;
    if (parentItem == rootItem.get())
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

QModelIndex ModelFrameInfoTree::index(
    int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem = parent.isValid() ? static_cast<TreeItem *>(parent.internalPointer())
                                            : rootItem.get();
    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

/**
 @class ModelFrameBinary
*/

int ModelFrameBinary::rowCount(
    const QModelIndex &parent) const
{
    return m_data.m_size / 16 + 1;
}
int ModelFrameBinary::columnCount(
    const QModelIndex &parent) const
{
    return 16;
}
QVariant ModelFrameBinary::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (m_data.m_size / 16 + 1) || index.column() >= 16)
    {
        return QVariant();
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    int row = index.row();
    int column = index.column();

    if (row * 16 + column < m_data.m_size)
        return QString("%1").arg(QString::number((uint)m_data.m_bin_data[row * 16 + column], 16)
                                     .rightJustified(2, '0')
                                     .toUpper());
    return QString();
}

QVariant ModelFrameBinary::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
    {
        // 根据列索引返回相应的表头数据
        char header[] = "0123456789abcdef";
        return QString(header[section]);
    }

    if (orientation == Qt::Vertical)
    {
        // 根据列索引返回相应的表头数据=
        if (section < (m_data.m_size / 16 + 1))
            return QString("%1").arg(
                QString::number((uint)(m_data.m_offset + section * 16), 16).rightJustified(8, '0'));
    }

    return QVariant();
}
