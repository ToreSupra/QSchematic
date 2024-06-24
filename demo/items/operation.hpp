#pragma once

#include "itemtypes.hpp"

#include <qschematic/items/node.hpp>

#include <boost/serialization/split_member.hpp>

namespace QSchematic::Items
{
    class Label;
}

class Operation : public QSchematic::Items::Node
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Operation)

public:
    explicit Operation(int type = ::ItemType::OperationType, QGraphicsItem* parent = nullptr);
    ~Operation() override;

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive& ar, const unsigned int version) const;
    template<class Archive>
    void load(Archive& ar, const unsigned int version);
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    std::shared_ptr<QSchematic::Items::Item> deepCopy() const override;
    std::unique_ptr<QWidget> popup() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void alignLabel();
    std::shared_ptr<QSchematic::Items::Label> label() const;
    void setText(const QString& text);
    QString text() const;

protected:
    void copyAttributes(Operation& dest) const;

private:
    std::shared_ptr<QSchematic::Items::Label> _label;
};

BOOST_CLASS_EXPORT_KEY(Operation)
