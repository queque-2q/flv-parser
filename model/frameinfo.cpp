#include <QMapIterator>
#include <QMessageBox>
#include <QSize>
#include <QBuffer>
#include <vector>
#include <QDebug>
#include "frameinfo.h"

// 解析AMF字符串
QString parseAMFString(QDataStream &stream) {
    quint16 strLen;
    stream >> strLen;
    
    QByteArray strData(strLen, 0);
    stream.readRawData(strData.data(), strLen);
    
    return QString::fromUtf8(strData);
}

// 解析AMF数字
double parseAMFNumber(QDataStream &stream) {
    double value;
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
    stream >> value;
    return value;
}

// 解析AMF布尔值
bool parseAMFBoolean(QDataStream &stream) {
    quint8 value;
    stream >> value;
    return value != 0;
}

// 解析AMF对象或ECMA数组
void DataFrameInfo::parseAMFObject(QDataStream &stream, bool isECMAArray) {
    // 如果是ECMA数组，跳过数组长度字段
    if (isECMAArray) {
        quint32 arrayLen;
        stream >> arrayLen;
    }
    
    while (!stream.atEnd()) {
        // 读取属性名
        QString propertyName = parseAMFString(stream);
        
        // 检查是否到达对象结束标记
        if (propertyName.isEmpty()) {
            quint8 endMarker;
            stream >> endMarker;
            if (endMarker == AMF_OBJECT_END)
                break;
            continue;
        }
        
        // 读取属性值类型
        quint8 valueType;
        stream >> valueType;
        
        // 根据类型解析值
        switch (valueType) {
            case AMF_NUMBER: {
                double value = parseAMFNumber(stream);
                m_metadata_values[propertyName] = QString::number(value);
                break;
            }
            case AMF_BOOLEAN: {
                bool value = parseAMFBoolean(stream);
                m_metadata_values[propertyName] = value ? "true" : "false";
                break;
            }
            case AMF_STRING: {
                QString value = parseAMFString(stream);
                m_metadata_values[propertyName] = value;
                break;
            }
            case AMF_NULL:
            case AMF_UNDEFINED:
                m_metadata_values[propertyName] = "null";
                break;
            default:
                // 对于其他复杂类型，简单跳过
                m_metadata_values[propertyName] = QString("未支持的类型: 0x%1").arg(valueType, 2, 16, QChar('0'));
                break;
        }
    }
}

// 解析AMF数据
void DataFrameInfo::ReadFromStream(QDataStream &stream) {
    try {
        // 读取第一个AMF包 - 通常是字符串，表示元数据类型
        quint8 type;
        stream >> type;
        
        if (type == AMF_STRING) {
            m_metadata_type = parseAMFString(stream);
        } else {
            m_metadata_type = QString("未知类型: 0x%1").arg(type, 2, 16, QChar('0'));
            qDebug() << "元数据类型不是字符串，而是:" << type;
        }
        
        // 读取第二个AMF包 - 通常是ECMA数组，包含实际元数据
        if (!stream.atEnd()) {
            stream >> type;
            if (type == AMF_ECMA_ARRAY) {
                parseAMFObject(stream, true);
            } else if (type == AMF_OBJECT) {
                parseAMFObject(stream);
            } else {
                qDebug() << "元数据值不是ECMA数组或对象，而是:" << type;
            }
        } else {
            qDebug() << "元数据包中只有类型，没有值";
        }
    } catch (const std::exception &e) {
        qDebug() << "解析AMF数据时发生异常:" << e.what();
        m_metadata_values["error"] = QString("解析错误: %1").arg(e.what());
    } catch (...) {
        qDebug() << "解析AMF数据时发生未知异常";
        m_metadata_values["error"] = "未知解析错误";
    }
}

