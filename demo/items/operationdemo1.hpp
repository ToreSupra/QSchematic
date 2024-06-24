#pragma once

#include "operation.hpp"

class OperationDemo1 : public Operation
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(OperationDemo1)

public:
    explicit OperationDemo1(QGraphicsItem* parent = nullptr);
    ~OperationDemo1() override = default;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version);
    std::shared_ptr<QSchematic::Items::Item> deepCopy() const override;

private:
    void copyAttributes(OperationDemo1& dest) const;
};

BOOST_CLASS_EXPORT_KEY(OperationDemo1)
