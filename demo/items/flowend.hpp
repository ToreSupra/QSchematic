#pragma once

#include "operation.hpp"

class OperationConnector;

class FlowEnd :
    public Operation
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FlowEnd)

public:
    FlowEnd();
    ~FlowEnd() override = default;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
    std::shared_ptr<Item> deepCopy() const override;
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    void copyAttributes(FlowEnd& dest) const;

private:
    QPolygon _symbolPolygon;
};

BOOST_CLASS_EXPORT_KEY(FlowEnd)
