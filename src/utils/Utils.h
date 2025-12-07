// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

constexpr int FLV_HEADER_SIZE = 13;

inline unsigned int bigend_ctou24(const unsigned char* data) {
    return *(data + 2) + (*(data + 1) << 8) + (*(data) << 16);
}

inline int bigend_ctoi24(const unsigned char* data) {
    return (int) (*(data + 2) + (*(data + 1) << 8) + (*(data) << 16));
}

inline unsigned int bigend_ctou32(const unsigned char* data) {
    return *(data + 3) + (*(data + 2) << 8) + (*(data + 1) << 16) + (*(data) << 24);
}

inline void read_exception(QDataStream& stream, char* buffer, int size) {
    if (size != stream.readRawData(buffer, size)) {
        throw QString("eof");
    }
}
