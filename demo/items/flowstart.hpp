#pragma once

#include "operation.hpp"

class FlowStart :
    public Operation
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FlowStart)

public:
    FlowStart();
    ~FlowStart() override = default;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
    std::shared_ptr<Item> deepCopy() const override;
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void copyAttributes(FlowStart& dest) const;

private:
    QPolygon _symbolPolygon;
};

BOOST_CLASS_EXPORT_KEY(FlowStart)
