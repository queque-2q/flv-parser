// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "BinaryEditDelegate.h"
#include "Log.h"
#include <QLineEdit>
#include <QMessageBox>

BinaryEditDelegate::BinaryEditDelegate(QObject* parent) : QStyledItemDelegate(parent) {
}

void BinaryEditDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (!lineEdit)
        return;

    QString newValue = lineEdit->text().trimmed().toUpper();
    QString oldValue = index.data(Qt::DisplayRole).toString();

    // 解析输入的十六进制字符串
    bool ok = false;
    uint8_t intNewValue = static_cast<uint8_t>(newValue.toUInt(&ok, 16));
    uint8_t intOldValue = static_cast<uint8_t>(oldValue.toUInt(&ok, 16));

    if (!ok || intNewValue > 0xFF) {
        QMessageBox::warning(editor->parentWidget(),
                             "无效输入",
                             QString("输入的值 [%1] 不是有效的十六进制字节（00-FF）。").arg(newValue));
        return;
    }

    // 如果值没有改变，直接返回
    if (intNewValue == intOldValue)
        return;

    // 弹出确认对话框
    QMessageBox::StandardButton reply = QMessageBox::question(
        editor->parentWidget(),
        "确认修改",
        QString("确定要将字节从 [0x%1] 修改为 [0x%2] 吗？\n\n此操作会直接写入文件！").arg(oldValue).arg(newValue),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 用户确认，调用基类方法写入数据
        QStyledItemDelegate::setModelData(editor, model, index);
    }
    // 如果用户选择 No，不做任何操作，编辑被取消
}
