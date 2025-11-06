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

bool StreamDeleteStrategy::deleteFrame(const QString& filePath,
                                       int rowIndex,
                                       const vector<shared_ptr<FLVFrame>>& frameList) {
    QString tempFileName = filePath + "_temp";

    try {
        // 复制未删除的帧到临时文件
        if (!copyFramesToTemp(filePath, tempFileName, rowIndex, frameList)) {
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

        return true;
    } catch (const QString& error) {
        QMessageBox::warning(nullptr, "错误", "删除帧失败：" + error);
        QFile::remove(tempFileName);
        return false;
    }
}

bool StreamDeleteStrategy::copyFramesToTemp(const QString& sourcePath,
                                            const QString& tempPath,
                                            int rowIndex,
                                            const vector<shared_ptr<FLVFrame>>& frameList) {
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
    for (size_t i = 0; i < frameList.size(); ++i) {
        if (i != rowIndex) {
            auto frame = frameList[i].get();
            sourceFile.seek(frame->m_offset);
            QByteArray frameData(frame->m_size, 0);
            size_t readSize = sourceFile.read(frameData.data(), frame->m_size);

            tempFile.write(frameData, readSize);

            if (readSize != frame->m_size) {
                qCDebug(runLog) << "文件结束";
            }
        }
    }

    sourceFile.close();
    tempFile.close();
    return true;
}

bool MMapDeleteStrategy::deleteFrame(const QString& filePath,
                                     int rowIndex,
                                     const vector<shared_ptr<FLVFrame>>& frameList) {
    return deleteFrameInMemory(filePath, rowIndex, frameList);
}

bool MMapDeleteStrategy::deleteFrameInMemory(const QString& filePath,
                                             int rowIndex,
                                             const vector<shared_ptr<FLVFrame>>& frameList) {
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
        auto& frames = frameList;
        int64_t startPos = frames[rowIndex]->m_offset;
        int64_t endPos = frames[rowIndex]->m_offset + frames[rowIndex]->m_size;
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
