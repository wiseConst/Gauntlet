#pragma once

namespace Eclipse
{
// Multithreading part. Base class non-copyable && non-moveable.
class Job
{
  public:
    Job() = default;
    virtual ~Job();

    Job(const Job& that) = delete;
    Job(Job&& that) = delete;

    Job& operator=(const Job& that) = delete;
    Job& operator=(Job&& that) = delete;
};

}  // namespace Eclipse