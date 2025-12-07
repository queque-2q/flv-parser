// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include "taginfo.h"
#include <QFile>
#include <QString>
#include <memory>
#include <vector>

using namespace std;

/**
 * @class TagDeleteStrategy
 * @brief 抽象基类，定义删除帧的策略接口
 */
class TagDeleteStrategy {
  public:
    virtual ~TagDeleteStrategy() = default;
    virtual bool deleteTag(const QString& filePath, int rowIndex, const vector<unique_ptr<FLVTag>>& tagList) = 0;
};

/**
 * @class StreamDeleteStrategy
 * @brief 流式删除策略，适用于小文件的删除操作
 */
class StreamDeleteStrategy : public TagDeleteStrategy {
  public:
    ~StreamDeleteStrategy() override = default;
    bool deleteTag(const QString& filePath, int rowIndex, const vector<unique_ptr<FLVTag>>& tagList) override;

  private:
    bool copyTagsToTemp(const QString& sourcePath,
                          const QString& tempPath,
                          int rowIndex,
                          const vector<unique_ptr<FLVTag>>& tagList);
};

/**
 * @class MMapDeleteStrategy
 * @brief 内存映射删除策略，适用于大文件的删除操作
 */
class MMapDeleteStrategy : public TagDeleteStrategy {
  public:
    ~MMapDeleteStrategy() override = default;
    bool deleteTag(const QString& filePath, int rowIndex, const vector<unique_ptr<FLVTag>>& tagList) override;

  private:
    bool deleteTagInMemory(const QString& filePath, int rowIndex, const vector<unique_ptr<FLVTag>>& tagList);
};

/**
 * @class TagDeleteStrategyFactory
 * @brief 工厂类，根据文件大小选择删除策略
 */
class TagDeleteStrategyFactory {
  public:
    static unique_ptr<TagDeleteStrategy> createStrategy(const QString& filePath) {
        QFile file(filePath);
        if (file.size() < 10 * 1024 * 1024) { // 10MB
            return make_unique<StreamDeleteStrategy>();
        } else {
            return make_unique<MMapDeleteStrategy>();
        }
    }
};
