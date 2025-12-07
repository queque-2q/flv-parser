// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include "docview.h"
#include "ui_docview.h"
#include <QFile>
#include <QTextStream>

DocView::DocView(QWidget* parent) : QWidget(parent), ui(new Ui::DocView) {
    ui->setupUi(this);
    loadDocument();
}

DocView::~DocView() {
    delete ui;
}

void DocView::loadDocument() {
    // 加载 HTML 文件
    QFile file(":/res/user_guide.html");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString html = QString::fromUtf8(file.readAll());
        ui->docTextBrowser->setHtml(html);
        file.close();
    }
}
