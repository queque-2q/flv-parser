// SPDX-FileCopyrightText: 2025 FLV Parser Contributors
//
// SPDX-License-Identifier: MIT

#pragma once

#include "frameinfo.h"
#include <QFile>
#include <QString>
#include <memory>
#include <vector>

using namespace std;

/**
 * @class FrameDeleteStrategy
 * @brief 抽象基类，定义删除帧的策略接口
 */
class FrameDeleteStrategy {
  public:
    virtual ~FrameDeleteStrategy() = default;
    virtual bool deleteFrame(const QString& filePath, int rowIndex, const vector<shared_ptr<FLVFrame>>& frameList) = 0;
};

/**
 * @class StreamDeleteStrategy
 * @brief 流式删除策略，适用于小文件的删除操作
 */
class StreamDeleteStrategy : public FrameDeleteStrategy {
  public:
    ~StreamDeleteStrategy() override = default;
    bool deleteFrame(const QString& filePath, int rowIndex, const vector<shared_ptr<FLVFrame>>& frameList) override;

  private:
    bool copyFramesToTemp(const QString& sourcePath,
                          const QString& tempPath,
                          int rowIndex,
                          const vector<shared_ptr<FLVFrame>>& frameList);
};

/**
 * @class MMapDeleteStrategy
 * @brief 内存映射删除策略，适用于大文件的删除操作
 */
class MMapDeleteStrategy : public FrameDeleteStrategy {
  public:
    ~MMapDeleteStrategy() override = default;
    bool deleteFrame(const QString& filePath, int rowIndex, const vector<shared_ptr<FLVFrame>>& frameList) override;

  private:
    bool deleteFrameInMemory(const QString& filePath, int rowIndex, const vector<shared_ptr<FLVFrame>>& frameList);
};

/**
 * @class FrameDeleteStrategyFactory
 * @brief 工厂类，根据文件大小选择删除策略
 */
class FrameDeleteStrategyFactory {
  public:
    static unique_ptr<FrameDeleteStrategy> createStrategy(const QString& filePath) {
        QFile file(filePath);
        if (file.size() < 10 * 1024 * 1024) { // 10MB
            return make_unique<StreamDeleteStrategy>();
        } else {
            return make_unique<MMapDeleteStrategy>();
        }
    }
};
