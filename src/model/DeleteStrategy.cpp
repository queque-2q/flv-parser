// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#include <windows.h>
#ifdef byte
#undef byte // 强制移除 byte 类型别名
#endif

#include "DeleteStrategy.h"
#include "Log.h"
#include "Utils.h"
#include <QFile>
#include <QMessageBox>

bool StreamDeleteStrategy::deleteTag(const QString& filePath,
                                       int rowIndex,
                                       const vector<unique_ptr<FLVTag>>& tagList) {
    QString tempFileName = filePath + "_temp";

    try {
        // 复制未删除的帧到临时文件
        if (!copyTagsToTemp(filePath, tempFileName, rowIndex, tagList)) {
            throw QString("复制帧数据失败");
        }

        // 删除源文件并重命名临时文件
        QFile sourceFile(filePath);
        QFile tempFile(tempFileName);

        if (!sourceFile.remove()) {
            throw QString("删除源文件失败");
        }
        if (!tempFile.rename(filePath)) {
            throw QString("重命名临时文件失败");
        }

        qCInfo(runLog) << "[flv-editing] event[tag deleted successfully]";
        QMessageBox::information(nullptr, "成功", "帧已成功删除");
        return true;
    } catch (const QString& error) {
        qCInfo(runLog) << "[flv-editing] event[tag deleted failed]";
        QMessageBox::warning(nullptr, "错误", "删除帧失败：" + error);
        QFile::remove(tempFileName);
        return false;
    }
}

bool StreamDeleteStrategy::copyTagsToTemp(const QString& sourcePath,
                                            const QString& tempPath,
                                            int rowIndex,
                                            const vector<unique_ptr<FLVTag>>& tagList) {
    QFile sourceFile(sourcePath);
    QFile tempFile(tempPath);

    if (!sourceFile.open(QIODevice::ReadOnly) || !tempFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    // 复制FLV头
    QByteArray header(FLV_HEADER_SIZE, 0);
    if (sourceFile.read(header.data(), FLV_HEADER_SIZE) != FLV_HEADER_SIZE) {
        return false;
    }
    tempFile.write(header);

    // 复制未删除的帧
    for (size_t i = 0; i < tagList.size(); ++i) {
        if (i != rowIndex) {
            auto tag = tagList[i].get();
            sourceFile.seek(tag->m_offset);
            QByteArray tagData(tag->m_size, 0);
            size_t readSize = sourceFile.read(tagData.data(), tag->m_size);

            tempFile.write(tagData, readSize);
        }
    }

    sourceFile.close();
    tempFile.close();
    return true;
}

bool MMapDeleteStrategy::deleteTag(const QString& filePath,
                                     int rowIndex,
                                     const vector<unique_ptr<FLVTag>>& tagList) {

    bool result = deleteTagInMemory(filePath, rowIndex, tagList);
    if (result) {
        qCInfo(runLog) << "[flv-editing] event[tag deleted successfully]";
        QMessageBox::information(nullptr, "成功", "帧已成功删除");
    } else {
        qCInfo(runLog) << "[flv-editing] event[tag deleted failed]";
        QMessageBox::warning(nullptr, "错误", "删除帧失败");
    }
    return result;
}

bool MMapDeleteStrategy::deleteTagInMemory(const QString& filePath,
                                             int rowIndex,
                                             const vector<unique_ptr<FLVTag>>& tagList) {
    HANDLE hFile = CreateFileW((LPCWSTR) filePath.utf16(),
                               GENERIC_READ | GENERIC_WRITE,
                               0,
                               nullptr,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    HANDLE hMapping = CreateFileMapping(hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    if (hMapping == nullptr) {
        CloseHandle(hFile);
        return false;
    }

    LPVOID pView = MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (pView == nullptr) {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return false;
    }

    try {
        // 计算需要移动的数据大小和位置
        auto& tags = tagList;
        int64_t startPos = tags[rowIndex]->m_offset;
        int64_t endPos = tags[rowIndex]->m_offset + tags[rowIndex]->m_size;
        int64_t moveSize = GetFileSize(hFile, nullptr) - endPos;

        // 移动数据
        if (moveSize > 0) {
            memmove((char*) pView + startPos, (char*) pView + endPos, moveSize);
        }

        // 设置新的文件大小
        SetFilePointer(hFile, startPos + moveSize, nullptr, FILE_BEGIN);
        SetEndOfFile(hFile);
    } catch (...) {
        UnmapViewOfFile(pView);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return false;
    }

    UnmapViewOfFile(pView);
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return true;
}
