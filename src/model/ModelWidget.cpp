#include "ModelWidget.h"
#include "Log.h"
#include "Utils.h"
#include <QApplication>
#include <QBuffer>
#include <QMessageBox>
#include <QSize>
#include <vector>

int ModelFrameList::rowCount(const QModelIndex& parent) const {
    return m_frameList.size();
}
int ModelFrameList::columnCount(const QModelIndex& parent) const {
    return ModelFrameList::column_size;
}

QVariant ModelFrameList::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_frameList.size() || index.column() >= ModelFrameList::column_size) {
        return {};
    }

    if (role == Qt::DisplayRole) {
        // 定义枚举映射
        static QMap<uint8_t, const char*> tagTypeMap = {
            {TAG_TYPE_AUDIO, "音频"}, {TAG_TYPE_VIDEO, "视频"}, {TAG_TYPE_METADATA, "元数据"}};
        // 枚举映射结束

        int row = index.row();
        int column = index.column();

        switch (column) {
        case 0:
            return QString("0x%1").arg(QString::number(uint(m_frameList[row]->m_offset), 16).rightJustified(8, '0'));
        case 1:
            return QString("%1 (%2)")
                .arg(tagTypeMap[get<double>(m_frameList[row]->m_tag_type->value)])
                .arg(get<double>(m_frameList[row]->m_tag_type->value));
        case 2:
            return QString("%1").arg(m_frameList[row]->m_size);
        case 3:
            return QString("%1").arg(get<double>(m_frameList[row]->m_timestamp->value));
        case 4:
        default:
            return {};
        }
    }

    if (role == Qt::BackgroundRole) {
        // 根据应用的调色板判断当前是深色主题还是浅色主题，然后返回不同的颜色
        QColor windowColor = qApp->palette().color(QPalette::Window);
        bool darkTheme = windowColor.lightnessF() < 0.5; // lightnessF 返回 0.0 - 1.0

        auto makeTagColor = [&](int hue) -> QColor {
            if (darkTheme) {
                // 深色主题：使用较暗、饱和度中等的色块
                return QColor::fromHsv(hue, 80, 95);
            } else {
                // 浅色主题：使用低饱和度、高明度的柔和色块
                return QColor::fromHsv(hue, 30, 245);
            }
        };

        int tag = static_cast<int>(get<double>(m_frameList[index.row()]->m_tag_type->value));
        if (tag == TAG_TYPE_METADATA)
            return makeTagColor(240);
        if (tag == TAG_TYPE_AUDIO)
            return makeTagColor(120);
        if (tag == TAG_TYPE_VIDEO)
            return makeTagColor(60);
    }

    return {};
}

QVariant ModelFrameList::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        // 根据列索引返回相应的表头数据
        array<const char*, ModelFrameList::column_size> header = {"偏移地址", "类型", "长度", "时间戳", "详细信息"};
        if (section < ModelFrameList::column_size)
            return QString(header[section]);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int ModelFrameList::readFromFile(QFile& file) {
    QDataStream stream(&file);
    vector<shared_ptr<FLVFrame>> frame_vec;

    // 解析帧

    // flv头
    // QByteArray buffer(64, 0);
    unsigned char buffer[64] = {0};
    if (3 != stream.readRawData(reinterpret_cast<char*>(buffer), 3)) {
        throw QString("文件格式错误");
    }

    if (0 != memcmp(buffer, "FLV", 3)) {
        throw QString("文件格式错误");
    }

    stream.skipRawData(10);

    try {
        int64_t len = 0;
        while (!stream.atEnd()) {
            shared_ptr<FLVFrame> frame_info = make_shared<FLVFrame>();

            frame_info->m_offset = file.pos();
            uint8_t tag_type = 0;
            stream >> tag_type;
            frame_info->m_tag_type->value = (double) tag_type; // metadata,音视频

            if (3 != stream.readRawData(reinterpret_cast<char*>(buffer), 3))
                throw QString("文件结束");
            frame_info->m_tag_size->value = (double) bigend_ctou24(buffer);
            frame_info->m_size = get<double>(frame_info->m_tag_size->value) + 11 + 4;

            if (3 != stream.readRawData(reinterpret_cast<char*>(buffer), 3))
                throw QString("文件结束");
            frame_info->m_timestamp->value = (double) bigend_ctou24(buffer);
            if (1 != stream.readRawData(reinterpret_cast<char*>(buffer), 1))
                throw QString("文件结束");
            frame_info->m_timestamp->value =
                get<double>(frame_info->m_timestamp->value) + (static_cast<uint32_t>(buffer[0]) << 24);

            if (3 != stream.readRawData(reinterpret_cast<char*>(buffer), 3))
                throw QString("文件结束");
            frame_info->m_stream_id->value = (double) bigend_ctou24(buffer);

            switch (static_cast<int>(get<double>(frame_info->m_tag_type->value))) {
            case TAG_TYPE_METADATA: {
                // 读取metadata
                frame_info->metadata_info = make_unique<DataFrameInfo>(frame_info.get());
                // 使用AMF解析函数解析元数据
                frame_info->metadata_info->ReadFromStream(stream);
            } break;
            case TAG_TYPE_AUDIO: {
                frame_info->a_info = make_unique<AudioFrameInfo>(frame_info.get());
                /*uint8_t m_sound_format;
                    uint8_t m_sound_rate;
                    uint8_t m_sound_size;
                    uint8_t m_sound_type;
                    int m_detail_type;*/

                // 读取音频帧头信息(1字节)
                if (1 != stream.readRawData(reinterpret_cast<char*>(buffer), 1))
                    throw QString("文件结束");
                frame_info->a_info->m_sound_format->value = (double) ((buffer[0] & 0xF0) >> 4); // 高4位
                frame_info->a_info->m_sound_rate->value = (double) ((buffer[0] & 0x0C) >> 2);   // 3-2位
                frame_info->a_info->m_sound_size->value = (double) ((buffer[0] & 0x02) >> 1);   // 1位
                frame_info->a_info->m_sound_type->value = (double) (buffer[0] & 0x01);          // 0位

                if (get<double>(frame_info->a_info->m_sound_format->value) == AAC) {
                    // AAC
                    if (1 != stream.readRawData(reinterpret_cast<char*>(buffer), 1))
                        throw QString("文件结束");
                    frame_info->a_info->m_detail_type->value = (double) buffer[0];
                }
            } break;
            case TAG_TYPE_VIDEO: {
                frame_info->v_info = make_unique<VideoFrameInfo>(frame_info.get());
                /*uint8_t m_frame_type;
                    uint8_t m_codec;
                    int m_cts;
                    int m_detail_type;*/

                // 读取视频帧头信息(1字节)
                if (1 != stream.readRawData(reinterpret_cast<char*>(buffer), 1))
                    throw QString("文件结束");
                frame_info->v_info->m_frame_type->value = (double) ((buffer[0] & 0xF0) >> 4); // 高4位
                frame_info->v_info->m_codec->value = (double) (buffer[0] & 0x0F);             // 低4位

                // 如果是AVC(H.264)需要额外读取CTS
                if (get<double>(frame_info->v_info->m_codec->value) == AVC ||
                    get<double>(frame_info->v_info->m_codec->value) == HEVC ||
                    get<double>(frame_info->v_info->m_codec->value) == AV1 ||
                    get<double>(frame_info->v_info->m_codec->value) == VVC) {
                    if (1 != stream.readRawData(reinterpret_cast<char*>(buffer), 1))
                        throw QString("文件结束");
                    frame_info->v_info->m_detail_type->value = (double) buffer[0];

                    if (3 != stream.readRawData(reinterpret_cast<char*>(buffer), 3))
                        throw QString("文件结束");
                    frame_info->v_info->m_cts->value = static_cast<double>(bigend_ctoi24(buffer));
                }
            } break;
            default:
                break;
            }

            // 读取previous_tag_size
            frame_info->m_previous_tag_size->offset = 11 + (int) get<double>(frame_info->m_tag_size->value);
            file.seek(frame_info->m_offset + frame_info->m_previous_tag_size->offset);
            if (4 != stream.readRawData(reinterpret_cast<char*>(buffer), 4))
                throw QString("文件结束");
            frame_info->m_previous_tag_size->value = (double) bigend_ctou32(buffer);

            // 读取帧的二进制数据
            frame_info->m_bin_data.reset(new uchar[frame_info->m_previous_tag_size->offset + 4]);
            file.seek(frame_info->m_offset);
            stream.readRawData((char*) frame_info->m_bin_data.get(), frame_info->m_previous_tag_size->offset + 4);

            frame_vec.emplace_back(frame_info);
        }
    } catch (QString& e) {
        qDebug(runLog) << "读取帧列表:" << e;
    }

    m_frameList.swap(frame_vec);
    return 0;
}

/**
 @class ModelFrameInfoTree
*/

QVariant ModelFrameInfoTree::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (role == Qt::SizeHintRole)
        return QSize(0, 23);

    if (role == Qt::DisplayRole) {
        auto item = static_cast<TreeItem*>(index.internalPointer()); // 怎么得到internal pointer?

        if (index.column() == 0)
            return item->data->name;
        if (index.column() == 1)
            return item->data->toStringFunc(*item->data);
    }
    return {};
}

QModelIndex ModelFrameInfoTree::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return {};

    auto childItem = static_cast<TreeItem*>(index.internalPointer());
    if (childItem == nullptr || childItem == m_frame_info.get())
        return {};

    auto parentItem = childItem->parentItem;
    return createIndex(parentItem->row(), 0, parentItem);
}

QModelIndex ModelFrameInfoTree::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent))
        return {};

    TreeItem* parentItem = parent.isValid() ? static_cast<TreeItem*>(parent.internalPointer()) : m_frame_info.get();
    TreeItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return {};
}

/**
 @class ModelFrameBinary
*/

int ModelFrameBinary::rowCount(const QModelIndex& parent) const {
    return m_data.m_size / 16 + 1;
}

int ModelFrameBinary::columnCount(const QModelIndex& parent) const {
    return 16;
}

QVariant ModelFrameBinary::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= (m_data.m_size / 16 + 1) || index.column() >= 16) {
        return {};
    }

    if (role != Qt::DisplayRole)
        return {};

    int row = index.row();
    int column = index.column();

    if (row * 16 + column < m_data.m_size)
        return QString("%1").arg(
            QString::number((uint) m_data.m_bin_data[row * 16 + column], 16).rightJustified(2, '0').toUpper());
    return {};
}

QVariant ModelFrameBinary::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Horizontal) {
        // 根据列索引返回相应的表头数据
        char header[] = "0123456789abcdef";
        return QString(header[section]);
    }

    if (orientation == Qt::Vertical) {
        // 根据列索引返回相应的表头数据
        if (section < (m_data.m_size / 16 + 1))
            return QString("%1").arg(
                QString::number((uint) (m_data.m_offset + section * 16), 16).rightJustified(8, '0'));
    }

    return {};
}
