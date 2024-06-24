#pragma once

#include <qschematic/items/wireroundedcorners.hpp>

class FancyWire : public QSchematic::Items::WireRoundedCorners
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(FancyWire)

public:
    explicit FancyWire(QGraphicsItem* parent = nullptr);
    ~FancyWire() override = default;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);
    std::shared_ptr<QSchematic::Items::Item> deepCopy() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void copyAttributes(FancyWire& dest) const;
};

BOOST_CLASS_EXPORT_KEY(FancyWire)
