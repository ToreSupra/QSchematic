#pragma once

#include "item.hpp"

#include <QFont>

#include <boost/serialization/export.hpp>

namespace QSchematic::Items
{

    class Label :
        public Item
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(Label)

        friend class CommandLabelRename;

    Q_SIGNALS:
        void textChanged(const QString& newText);
        void doubleClicked();

    public:
        Label(int type = Item::LabelType, QGraphicsItem* parent = nullptr);
        ~Label() override = default;

        friend class ::boost::serialization::access;
        template<class Archive>
        void save(Archive& ar, const unsigned int version) const;
        template<class Archive>
        void load(Archive& ar, const unsigned int version);
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        std::shared_ptr<Item> deepCopy() const override;

        QRectF boundingRect() const final;
        QPainterPath shape() const final;

        void setText(const QString& text);
        QString text() const;
        void setFont(const QFont& font);
        QFont font() const;
        void setHasConnectionPoint(bool enabled);
        bool hasConnectionPoint() const;
        void setConnectionPoint(const QPointF& connectionPoint);    // Parent coordinates
        QRectF textRect() const;

    protected:
        void copyAttributes(Label& dest) const;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    private:
        void calculateTextRect();

        QString _text;
        QFont _font;
        QRectF _textRect;
        bool _hasConnectionPoint;
        QPointF _connectionPoint;   // Parent coordinates
    };

}

BOOST_CLASS_EXPORT_KEY(QSchematic::Items::Label)
