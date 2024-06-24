#pragma once

#include "item.hpp"
#include "../types.hpp"

#include <QList>

#include <boost/serialization/export.hpp>

class QGraphicsSceneMouseEvent;
class QGraphicsSceneHoverEvent;

namespace QSchematic::Items
{

    class RectItem :
        public Item
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(RectItem)

    Q_SIGNALS:
        void sizeChanged();

    public:
        enum Mode {
            None,
            Resize,
            Rotate,
        };
        Q_ENUM(Mode)

        explicit
        RectItem(int type, QGraphicsItem* parent = nullptr);

        ~RectItem() override = default;

        friend class ::boost::serialization::access;
        template<class Archive>
        void save(Archive& ar, const unsigned int version) const;
        template<class Archive>
        void load(Archive& ar, const unsigned int version);
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        std::shared_ptr<Item> deepCopy() const override;

        Mode mode() const;
        void setMinimumSize(const QSizeF& size);
        void setSize(QSizeF size);
        void setSize(qreal width, qreal height);
        void setWidth(qreal width);
        void setHeight(qreal height);
        QSizeF size() const;
        QRectF sizeRect() const;
        qreal width() const;
        qreal height() const;
        QRectF sceneRect() const;
        void setAllowMouseResize(bool enabled);
        void setAllowMouseRotate(bool enabled);
        bool allowMouseResize() const;
        bool allowMouseRotate() const;

        /**
         * @brief not really an event per se, but this seems the best way to
         * name it, all things considered. There are many items currently
         * that need to react to size-change and signal to self is boilerplate
         */
        virtual void sizeChangedEvent(QSizeF oldSize, QSizeF newSize);

        void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
        void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
        void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
        void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
        QRectF boundingRect() const override;
        QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value) override;
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        bool canSnapToGrid() const;
        void update() override;

    protected:
        void copyAttributes(RectItem& dest) const;
        QMap<RectanglePoint, QRectF> resizeHandles() const;
        QRectF rotationHandle() const;
        virtual void paintResizeHandles(QPainter& painter);
        virtual void paintRotateHandle(QPainter& painter);

    private:
        Mode _mode;
        QPointF _lastMousePosWithGridMove;
        RectanglePoint _resizeHandle;
        QSizeF _minimumSize;
        QSizeF _size;
        bool _allowMouseResize;
        bool _allowMouseRotate;
    };

}

BOOST_CLASS_EXPORT_KEY(QSchematic::Items::RectItem)

namespace boost {
    namespace serialization {
        template<class Archive>
        inline void save_construct_data(Archive& ar, const QSchematic::Items::RectItem* t, const unsigned int file_version);
        template<class Archive>
        inline void load_construct_data(Archive& ar, QSchematic::Items::RectItem* t, const unsigned int file_version);
    }
}