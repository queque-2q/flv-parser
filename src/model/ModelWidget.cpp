// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "ModelWidget.h"
#include "Log.h"
#include "Utils.h"
#include <QApplication>
#include <QBuffer>
#include <QMessageBox>
#include <QSize>
#include <vector>

int ModelTagList::rowCount(const QModelIndex& parent) const {
    return m_tagList.size() + (m_flv_header ? 1 : 0);
}
int ModelTagList::columnCount(const QModelIndex& parent) const {
    return ModelTagList::column_size;
}

QVariant ModelTagList::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.column() >= ModelTagList::column_size) {
        return {};
    }

    int row = index.row();
    int column = index.column();

    bool is_header_row = false;
    if (m_flv_header) {
        if (row == 0)
            is_header_row = true;
        else
            row -= 1; // 有header行时，数据行索引需要减1
    }

    if (row < 0 || row >= m_tagList.size()) {
        return {};
    }

    if (role == Qt::DisplayRole) {
        if (is_header_row) {
            switch (index.column()) {
            case 0:
                return QString("0x00000000");
            case 1:
                return QString("FLV Header");
            case 2:
                return QString("%1").arg(m_flv_header->m_size);
            default:
                return {};
            }
        }

        // 定义枚举映射
        static QMap<uint8_t, const char*> tagTypeMap = {
            {TAG_TYPE_AUDIO, "Audio"}, {TAG_TYPE_VIDEO, "Video"}, {TAG_TYPE_SCRIPT, "Script"}};
        // 枚举映射结束

        switch (column) {
        case 0:
            return QString("0x%1").arg(QString::number(m_tagList[row]->m_offset, 16).rightJustified(8, '0'));
        case 1:
            return QString("%1 (%2)")
                .arg(tagTypeMap[get<double>(m_tagList[row]->m_tag_type->value)])
                .arg(get<double>(m_tagList[row]->m_tag_type->value));
        case 2:
            return QString("%1").arg(m_tagList[row]->m_size);
        case 3:
            return QString("%1").arg(get<double>(m_tagList[row]->m_timestamp->value), 0, 'g', 10);
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

        if (is_header_row) {
            return makeTagColor(240);
        }

        int tag = static_cast<int>(get<double>(m_tagList[row]->m_tag_type->value));
        if (tag == TAG_TYPE_SCRIPT)
            return makeTagColor(180);
        if (tag == TAG_TYPE_AUDIO)
            return makeTagColor(120);
        if (tag == TAG_TYPE_VIDEO)
            return makeTagColor(60);
    }

    return {};
}

QVariant ModelTagList::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        // 根据列索引返回相应的表头数据
        array<const char*, ModelTagList::column_size> header = {"偏移地址", "tag类型", "长度", "时间戳", "详细信息"};
        if (section < ModelTagList::column_size)
            return QString(header[section]);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

int ModelTagList::readFromFile(QFile& file) {
    QDataStream stream(&file);
    vector<unique_ptr<FLVTag>> tag_vec;

    auto flv_header = make_unique<FLVHeader>();
    if (!flv_header->readfromStream(stream)) {
        return 0;
    }
    m_flv_header = std::move(flv_header);

    // 读取tag
    while (!stream.atEnd()) {
        unique_ptr<FLVTag> tag_info = make_unique<FLVTag>();
        if (!tag_info->readfromStream(stream)) {
            break;
        }

        tag_vec.emplace_back(std::move(tag_info));
    }

    m_tagList.swap(tag_vec);
    return 0;
}

/**
 @class ModelTagInfoTree
*/

QVariant ModelTagInfoTree::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (role == Qt::SizeHintRole)
        return QSize(0, 23);

    if (role == Qt::DisplayRole) {
        auto item = static_cast<TreeItem*>(index.internalPointer());

        if (index.column() == 0)
            return item->data->name;
        if (index.column() == 1)
            return item->data->toStringFunc(*item->data);
    }
    return {};
}

QModelIndex ModelTagInfoTree::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return {};

    auto childItem = static_cast<TreeItem*>(index.internalPointer());
    if (childItem == nullptr || childItem == m_tag_info.get())
        return {};

    auto parentItem = childItem->parentItem;
    return createIndex(parentItem->row(), 0, parentItem);
}

QModelIndex ModelTagInfoTree::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent))
        return {};

    TreeItem* parentItem = parent.isValid() ? static_cast<TreeItem*>(parent.internalPointer()) : m_tag_info.get();
    TreeItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return {};
}

/**
 @class ModelTagBinary
*/

int ModelTagBinary::rowCount(const QModelIndex& parent) const {
    return m_data.m_size / 16 + 1;
}

int ModelTagBinary::columnCount(const QModelIndex& parent) const {
    return 16;
}

QVariant ModelTagBinary::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= (m_data.m_size / 16 + 1) || index.column() >= 16) {
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return {};

    int row = index.row();
    int column = index.column();

    if (row * 16 + column < m_data.m_size)
        return QString("%1").arg(
            QString::number((uint) m_data.m_bin_data[row * 16 + column], 16).rightJustified(2, '0').toUpper());
    return {};
}

bool ModelTagBinary::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    int row = index.row();
    int column = index.column();
    int offset = row * 16 + column;

    if (offset >= m_data.m_size)
        return false;

    // 解析输入的十六进制字符串
    QString inputStr = value.toString().trimmed().toUpper();
    bool ok = false;
    uint8_t newValue = static_cast<uint8_t>(inputStr.toUInt(&ok, 16));

    if (!ok || newValue > 0xFF) {
        qCInfo(runLog) << QString("[flv-editing] event[invalid-input] input[%1]").arg(inputStr);
        return false;
    }

    // 写入文件
    if (m_filePath.isEmpty()) {
        qCInfo(runLog) << QString("[flv-editing] event[file-not-exist]");
        return false;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadWrite)) {
        qCInfo(runLog) << QString("[flv-editing] event[file-open-failed]");
        return false;
    }

    file.seek(m_data.m_offset + offset);
    file.write(reinterpret_cast<const char*>(&newValue), 1);
    file.close();

    qCInfo(runLog) << QString("[flv-editing] event[byte-write] offset[0x%1] new-value[0x%2]")
                          .arg(QString::number(m_data.m_offset + offset, 16).rightJustified(8, '0'))
                          .arg(QString::number(newValue, 16).rightJustified(2, '0').toUpper());

    // 通知视图数据已改变
    emit dataModified();
    return true;
}

Qt::ItemFlags ModelTagBinary::flags(const QModelIndex& index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;

    int row = index.row();
    int column = index.column();
    int offset = row * 16 + column;

    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    // 只有有效数据范围内的单元格可编辑
    if (offset < m_data.m_size) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

QVariant ModelTagBinary::headerData(int section, Qt::Orientation orientation, int role) const {
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
            return QString("%1").arg(QString::number(m_data.m_offset + section * 16, 16).rightJustified(8, '0'));
    }

    return {};
}
