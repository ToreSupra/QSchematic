#include "rectitem.hpp"
#include "../utils.hpp"
#include "../scene.hpp"
#include "../commands/rectitem_resize.hpp"
#include "../commands/rectitem_rotate.hpp"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QtMath>

const QColor COLOR_HIGHLIGHTED   = QColor(Qt::blue);
const QColor COLOR_BODY_FILL     = QColor(Qt::green);
const QColor COLOR_BODY_BORDER   = QColor(Qt::black);
const qreal PEN_WIDTH            = 1.5;
const int DEFAULT_WIDTH          = 160;
const int DEFAULT_HEIGHT         = 240;
const int DEFAULT_MINIMUM_WIDTH  = 40;
const int DEFAULT_MINIMUM_HEIGHT = 40;

BOOST_CLASS_EXPORT_IMPLEMENT(QSchematic::Items::RectItem)

using namespace QSchematic::Items;

RectItem::RectItem(int type, QGraphicsItem* parent) :
    Item(type, parent),
    _mode(None),
    _size(DEFAULT_WIDTH, DEFAULT_HEIGHT),
    _minimumSize(DEFAULT_MINIMUM_WIDTH, DEFAULT_MINIMUM_HEIGHT),
    _allowMouseResize(true),
    _allowMouseRotate(true)
{
}

template void RectItem::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive& ar, const unsigned int version);
template void RectItem::serialize<boost::archive::binary_iarchive>(boost::archive::binary_iarchive& ar, const unsigned int version);
template void RectItem::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive& ar, const unsigned int version);
template void RectItem::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive& ar, const unsigned int version);

namespace boost {
    namespace serialization {
        template<class Archive>
        void save_construct_data(Archive& ar, const QSchematic::Items::RectItem* t, const unsigned int file_version)
        {
            int tmp = t->type();
            ar << boost::serialization::make_nvp("rectitem_type", tmp);
        }

        template<class Archive>
        void load_construct_data(Archive& ar, QSchematic::Items::RectItem* t, const unsigned int file_version)
        {
            int tmp;
            ar >> boost::serialization::make_nvp("rectitem_type", tmp);
            ::new(t)QSchematic::Items::RectItem(tmp);
        }
    }
}

template<class Archive>
void RectItem::save(Archive& ar, const unsigned int version) const
{
    Q_UNUSED(version)

    // Root
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Item);
    qreal w = size().width();
    ar & boost::serialization::make_nvp("width", w);
    qreal h = size().height();
    ar & boost::serialization::make_nvp("height", h);
    qreal mw = _minimumSize.width();
    ar & boost::serialization::make_nvp("minimum_width", mw);
    qreal mh = _minimumSize.height();
    ar & boost::serialization::make_nvp("minimum_height", mh);
    bool amr = allowMouseResize();
    ar & boost::serialization::make_nvp("allow_mouse_resize", amr);
    bool amrot = allowMouseRotate();
    ar & boost::serialization::make_nvp("allow_mouse_rotate", amrot);
}

template<class Archive>
void RectItem::load(Archive& ar, const unsigned int version)
{
    Q_UNUSED(version)

    // Root
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Item);
    qreal w;
    ar & boost::serialization::make_nvp("width", w);
    qreal h;
    ar & boost::serialization::make_nvp("height", h);
    qreal mw;
    ar & boost::serialization::make_nvp("minimum_width", mw);
    qreal mh;
    ar & boost::serialization::make_nvp("minimum_height", mh);
    bool amr;
    ar & boost::serialization::make_nvp("allow_mouse_resize", amr);
    bool amrot;
    ar & boost::serialization::make_nvp("allow_mouse_rotate", amrot);

    setSize(w, h);
    setMinimumSize(QSizeF(mw, mh));
    setAllowMouseResize(amr);
    setAllowMouseRotate(amrot);
}

std::shared_ptr<Item> RectItem::deepCopy() const
{
    auto clone = std::make_shared<RectItem>(type(), parentItem());
    copyAttributes(*clone);

    return clone;
}

void RectItem::copyAttributes(RectItem& dest) const
{
    // Base class
    Item::copyAttributes(dest);

    // Attributes
    dest._mode = _mode;
    dest._lastMousePosWithGridMove = _lastMousePosWithGridMove;
    dest._resizeHandle = _resizeHandle;
    dest._minimumSize = _minimumSize;
    dest._size = _size;
    dest._allowMouseResize = _allowMouseResize;
    dest._allowMouseRotate = _allowMouseRotate;
}

RectItem::Mode RectItem::mode() const
{
    return _mode;
}

void RectItem::setMinimumSize(const QSizeF& size)
{
    _minimumSize = size;
}

void RectItem::setSize(QSizeF size)
{
    // Honor minimum size
    size.setWidth(std::max(size.width(), _minimumSize.width()));
    size.setHeight(std::max(size.height(), _minimumSize.height()));

    // short circuit when no effective change at all times as a manner of policy
    if (size == _size)
        return;

    // Boundary checks
    if (size.width() < 1 || size.height() < 1)
        return;

    QSizeF oldSize = _size;

    prepareGeometryChange();

    _size = size;

    setTransformOriginPoint(sizeRect().center());

    sizeChangedEvent(oldSize, _size);
    Q_EMIT sizeChanged();
}

void RectItem::setSize(qreal width, qreal height)
{
    return setSize(QSizeF(width, height));
}

void RectItem::setWidth(qreal width)
{
    setSize( width, size().height() );
}

void RectItem::setHeight(qreal height)
{
    setSize( size().width(), height );
}

QSizeF RectItem::size() const
{
    return _size;
}

QRectF RectItem::sizeRect() const
{
    return QRectF(0, 0, _size.width(), _size.height());
}

qreal RectItem::width() const
{
    return _size.width();
}

qreal RectItem::height() const
{
    return _size.height();
}

QRectF RectItem::sceneRect() const
{
    return QRectF{ scenePos().x(), scenePos().y(), width(), height() };
}

void RectItem::setAllowMouseResize(bool enabled)
{
    _allowMouseResize = enabled;
}

void RectItem::setAllowMouseRotate(bool enabled)
{
    _allowMouseRotate = enabled;
}

bool RectItem::allowMouseResize() const
{
    return _allowMouseResize;
}

bool RectItem::allowMouseRotate() const
{
    return _allowMouseRotate;
}

QMap<RectanglePoint, QRectF> RectItem::resizeHandles() const
{
    QMap<RectanglePoint, QRectF> map;
    const int& resizeHandleSize = _settings.resizeHandleSize;

    const QRectF& r = sizeRect();

    // Corners
    map.insert(RectanglePoint::BottomRight, QRectF(r.bottomRight()+QPointF(1,1)-QPointF(resizeHandleSize, resizeHandleSize), QSizeF(2*resizeHandleSize, 2*resizeHandleSize)));
    map.insert(RectanglePoint::BottomLeft, QRectF(r.bottomLeft()+QPointF(1,1)-QPointF(resizeHandleSize, resizeHandleSize), QSizeF(2*resizeHandleSize, 2*resizeHandleSize)));
    map.insert(RectanglePoint::TopRight, QRectF(r.topRight()+QPointF(1,1)-QPointF(resizeHandleSize, resizeHandleSize), QSizeF(2*resizeHandleSize, 2*resizeHandleSize)));
    map.insert(RectanglePoint::TopLeft, QRectF(r.topLeft()+QPointF(1,1)-QPointF(resizeHandleSize, resizeHandleSize), QSizeF(2*resizeHandleSize, 2*resizeHandleSize)));

    // Sides
    if (r.topRight().x() - r.topLeft().x() > 7*resizeHandleSize) {
        map.insert(RectanglePoint::Top, QRectF(Utils::centerPoint(r.topRight(), r.topLeft())+QPointF(1,1)-QPointF(resizeHandleSize, resizeHandleSize), QSizeF(2*resizeHandleSize, 2*resizeHandleSize)));
        map.insert(RectanglePoint::Bottom, QRectF(Utils::centerPoint(r.bottomRight(), r.bottomLeft())+QPointF(1,1)-QPointF(resizeHandleSize, resizeHandleSize), QSizeF(2*resizeHandleSize, 2*resizeHandleSize)));
    }
    if (r.bottomLeft().y() - r.topLeft().y() > 7*resizeHandleSize) {
        map.insert(RectanglePoint::Right, QRectF(Utils::centerPoint(r.topRight(), r.bottomRight())+QPointF(1,0)-QPoint(resizeHandleSize, resizeHandleSize), QSize(2*resizeHandleSize, 2*resizeHandleSize)));
        map.insert(RectanglePoint::Left, QRectF(Utils::centerPoint(r.bottomLeft(), r.topLeft())+QPointF(1,0)-QPoint(resizeHandleSize, resizeHandleSize), QSize(2*resizeHandleSize, 2*resizeHandleSize)));
    }

    return map;
}

QRectF RectItem::rotationHandle() const
{
    const QRectF& r = sizeRect();
    const int& resizeHandleSize = _settings.resizeHandleSize;
    return QRectF(Utils::centerPoint(r.topRight(), r.topLeft())+QPointF(1,-resizeHandleSize*3)-QPointF(resizeHandleSize, resizeHandleSize), QSizeF(2*resizeHandleSize, 2*resizeHandleSize));
}

void RectItem::sizeChangedEvent(QSizeF oldSize, QSizeF newSize)
{
    Q_UNUSED(oldSize)
    Q_UNUSED(newSize)
    // default implementation is noop
}

void RectItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();

    // Let the base class handle selection and so on
    Item::mousePressEvent(event);

    // Presume no mode
    _mode = None;

    // Check if clicked on a resize handle
    if (isSelected() && _allowMouseResize) {
        auto handles = resizeHandles();
        auto it = handles.constBegin();
        while (it != handles.constEnd()) {
            if (it.value().contains(event->pos().toPoint())) {
                _mode = Resize;
                _lastMousePosWithGridMove = event->scenePos();
                _resizeHandle = it.key();
                break;
            }
            it++;
        }
    }

    // Rotation
    if (isSelected() && _allowMouseRotate) {
        if (rotationHandle().contains(event->pos().toPoint())) {
            _mode = Rotate;
        }
    }
}

void RectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();
    Item::mouseReleaseEvent(event);
    _mode = None;
}

void RectItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    Q_ASSERT( scene() );

    event->accept();

    QPointF newMousePos( event->scenePos() );

    switch (_mode) {
    case None:
    {
        Item::mouseMoveEvent(event);

        break;
    }

    case Resize:
    {
        // Sanity check
        if (!_allowMouseResize) {
            qFatal("RectItem::mouseMoveEvent(): _mode is 'Resize' although _allowMouseResize is false");
            break;
        }

        // Left mouse button to move
        if (event->buttons() & Qt::LeftButton) {

            if ( canSnapToGrid() ) {
                newMousePos = _settings.snapToGrid( newMousePos );
            }

            // Calculate mouse movement in grid units
            QPointF d( newMousePos - _lastMousePosWithGridMove );

            // Rotate mouse movement
            {
                qreal angle = 2*M_PI - rotation() * M_PI / 180;
                qreal x = qCos(angle) * d.rx() - qSin(angle) * d.ry();
                qreal y = qSin(angle) * d.rx() + qCos(angle) * d.ry();
                d = QPointF(x, y);
            }

            qreal dx = d.x();
            qreal dy = d.y();

            // Don't do anything if there's nothing to do
            if (qFuzzyIsNull(dx) && qFuzzyIsNull(dy)) {
                break;
            }

            // Track this
            _lastMousePosWithGridMove = newMousePos;

            // Perform resizing
            qreal newX = posX();
            qreal newY = posY();
            qreal newWidth = _size.width();
            qreal newHeight = _size.height();
            switch (_resizeHandle) {
            case RectanglePoint::TopLeft:
                newX += dx;
                newY += dy;
                newWidth -= dx;
                newHeight -= dy;
                break;

            case RectanglePoint::Top:
                newY += dy;
                newHeight -= dy;
                break;

            case RectanglePoint::TopRight:
                newY += dy;
                newWidth += dx;
                newHeight -= dy;
                break;

            case RectanglePoint::Right:
                newWidth += dx;
                break;

            case RectanglePoint::BottomRight:
                newWidth += dx;
                newHeight += dy;
                break;

            case RectanglePoint::Bottom:
                newHeight += dy;
                break;

            case RectanglePoint::BottomLeft:
                newX += dx;
                newWidth -= dx;
                newHeight += dy;
                break;

            case RectanglePoint::Left:
                newX += dx;
                newWidth -= dx;
                break;
            }

            // Snap to grid (if supposed to)
            QPointF newPos( newX, newY );
            QSizeF newSize( newWidth, newHeight );
            if ( canSnapToGrid() ) {
                newSize = _settings.snapToGrid( newSize );
            }

            // Minimum size
            if (newSize.height() < _minimumSize.height()) {
                newSize.setHeight(_minimumSize.height());
                if (!qFuzzyCompare(newPos.ry(), pos().ry())) {
                    newPos.setY(posY() + _size.height() - _minimumSize.height());
                }
            }
            if (newSize.width() < _minimumSize.width()) {
                newSize.setWidth(_minimumSize.width());
                if (!qFuzzyCompare(newPos.rx(), pos().rx())) {
                    newPos.setX(posX() + _size.width() - _minimumSize.width());
                }
            }

            // Correct origin
            auto newOrigin = QPointF(newSize.width()/2, newSize.height()/2)+newPos - pos();
            auto angle = rotation() * M_PI / 180;
            auto offset = newOrigin - transformOriginPoint();
            auto newOriginRotated = QPointF(qCos(angle) * offset.rx() - qSin(angle) * offset.ry(), qSin(angle) * offset.rx() + qCos(angle) * offset.ry());
            auto correction = newOriginRotated - offset;
            newPos += correction;

            // Apply
            scene()->undoStack()->push(new Commands::RectItemResize(this, newPos, newSize));
        }

        break;
    }
    case Rotate:
    {
        // Sanity check
        if (!_allowMouseRotate) {
            qFatal("RectItem::mouseMoveEvent(): _mode is 'Rotate' although _allowMouseRotate is false");
            break;
        }

        auto center = sizeRect().center() + pos();
        auto delta = center - newMousePos;
        auto angle = fmod(qAtan2(delta.ry(), delta.rx())*180/M_PI + 270, 360);
        if (QApplication::keyboardModifiers() == Qt::ShiftModifier) {
            angle = qRound(angle/15)*15;
        }

        // Apply
        scene()->undoStack()->push(new Commands::RectItemRotate(this, angle));
    }
    }
}

void RectItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Item::hoverEnterEvent(event);
}

void RectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Item::hoverLeaveEvent(event);

    unsetCursor();
}

void RectItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    Item::hoverMoveEvent(event);

    // Set the cursor
    {
        setCursor(Qt::ArrowCursor);

        // If selected, we should adjust the cursor for the resize handles
        if (isSelected() && _allowMouseResize) {
            auto handles = resizeHandles();
            auto it = handles.constBegin();
            while (it != handles.constEnd()) {
                if (it.value().contains(event->pos().toPoint())) {
                    switch (it.key()) {
                    case RectanglePoint::TopLeft:
                    case RectanglePoint::BottomRight:
                        setCursor(Qt::SizeFDiagCursor);
                        break;

                    case RectanglePoint::Bottom:
                    case RectanglePoint::Top:
                        setCursor(Qt::SizeVerCursor);
                        break;

                    case RectanglePoint::BottomLeft:
                    case RectanglePoint::TopRight:
                        setCursor(Qt::SizeBDiagCursor);
                        break;

                    case RectanglePoint::Right:
                    case RectanglePoint::Left:
                        setCursor(Qt::SizeHorCursor);
                        break;
                    }
                    break;
                }
                it++;
            }
        }
        if (isSelected() && _allowMouseRotate) {
            if (rotationHandle().contains(event->pos().toPoint())) {
                setCursor(Qt::SizeAllCursor);
            }
        }
    }
}

QRectF RectItem::boundingRect() const
{
    // Body rect
    QRectF rect = QRectF(QPoint(0, 0), _size);
    qreal adj = 0.0;

    // Add half the pen width
    adj = qMax(adj, PEN_WIDTH / 2.0);

    // Add resize handles
    if (isSelected() && _allowMouseResize) {
        adj = qMax(adj, static_cast<qreal>(_settings.resizeHandleSize));
    }

    // Add highlight rect
    if (isHighlighted()) {
        adj = qMax(adj, static_cast<qreal>(_settings.highlightRectPadding));
    }

    // adjustment should be done before union with other rects, otherwise the
    // relative increase is added to outliers too
    rect = rect.adjusted(-adj, -adj, adj, adj);

    // Rotate handle
    if (isSelected() && _allowMouseRotate) {
        rect = rect.united(rotationHandle());
    }

    return rect;
}

void RectItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    // Draw the bounding rect if debug mode is enabled
    if (_settings.debug) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(Qt::red));
        painter->drawRect(boundingRect());
    }

    // Highlight rectangle
    if (isHighlighted()) {
        // Highlight pen
        QPen highlightPen;
        highlightPen.setStyle(Qt::NoPen);

        // Highlight brush
        QBrush highlightBrush;
        highlightBrush.setStyle(Qt::SolidPattern);
        highlightBrush.setColor(COLOR_HIGHLIGHTED);

        // Highlight rectangle
        painter->setPen(highlightPen);
        painter->setBrush(highlightBrush);
        painter->setOpacity(0.5);
        int adj = _settings.highlightRectPadding;
        painter->drawRoundedRect(sizeRect().adjusted(-adj, -adj, adj, adj), _settings.gridSize/2, _settings.gridSize/2);
    }

    painter->setOpacity(1.0);

    // Body pen
    QPen bodyPen;
    bodyPen.setWidthF(PEN_WIDTH);
    bodyPen.setStyle(Qt::SolidLine);
    bodyPen.setColor(COLOR_BODY_BORDER);

    // Body brush
    QBrush bodyBrush;
    bodyBrush.setStyle(Qt::SolidPattern);
    bodyBrush.setColor(COLOR_BODY_FILL);

    // Draw the component body
    painter->setPen(bodyPen);
    painter->setBrush(bodyBrush);
    painter->drawRoundedRect(sizeRect(), _settings.gridSize/2, _settings.gridSize/2);

    // Resize handles
    if (isSelected() && allowMouseResize()) {
        paintResizeHandles(*painter);
    }

    // Rotate handle
    if (isSelected() && allowMouseRotate()) {
        paintRotateHandle(*painter);
    }
}

void RectItem::update()
{
    // The item class sets the origin to the center of the bounding box
    // but in this case we want to rotate around the center of the body
    setTransformOriginPoint(sizeRect().center());
    QGraphicsObject::update();
}

bool RectItem::canSnapToGrid() const
{
    // Only snap when the rotation is a multiple of 90
    return Item::snapToGrid() && fmod(rotation(), 90) == 0;
}

QVariant RectItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    switch (change)
    {
    case QGraphicsItem::ItemPositionChange:
    {
        QPointF newPos = value.toPointF();
        if (canSnapToGrid()) {
            // If it is rotated 90 or 270 degrees and the difference between
            // the height and width is odd then the position needs to be
            // offset by half a grid unit vertically and horizontally.
            if ((qFuzzyCompare(qAbs(rotation()), 90) || qFuzzyCompare(qAbs(rotation()), 270)) &&
                (fmod(_size.width()/_settings.gridSize - _size.height()/_settings.gridSize, 2) != 0))
            {
                newPos.setX(qCeil(newPos.rx()/_settings.gridSize)*_settings.gridSize);
                newPos.setY(qCeil(newPos.ry()/_settings.gridSize)*_settings.gridSize);
                newPos -= QPointF(_settings.gridSize/2, _settings.gridSize/2);
            } else {
                newPos = _settings.snapToGrid(newPos);
            }
        }
        return newPos;
    }

    default:
        return Item::itemChange(change, value);
    }
}

void RectItem::paintResizeHandles(QPainter& painter)
{
    for (const QRectF& rect : resizeHandles()) {
        // Handle pen
        QPen handlePen;
        handlePen.setStyle(Qt::NoPen);
        painter.setPen(handlePen);

        // Handle Brush
        QBrush handleBrush;
        handleBrush.setStyle(Qt::SolidPattern);
        painter.setBrush(handleBrush);

        // Draw the outer handle
        handleBrush.setColor("#3fa9f5");
        painter.setBrush(handleBrush);
        painter.drawRect(rect.adjusted(-handlePen.width(), -handlePen.width(), handlePen.width()/2, handlePen.width()/2));

        // Draw the inner handle
        int adj = _settings.resizeHandleSize/2;
        handleBrush.setColor(Qt::white);
        painter.setBrush(handleBrush);
        painter.drawRect(rect.adjusted(-handlePen.width()+adj, -handlePen.width()+adj, (handlePen.width()/2)-adj, (handlePen.width()/2)-adj));
    }
}

void RectItem::paintRotateHandle(QPainter& painter)
{
    auto rect = rotationHandle();

    // Handle pen
    QPen handlePen;
    handlePen.setStyle(Qt::NoPen);
    painter.setPen(handlePen);

    // Handle Brush
    QBrush handleBrush;
    handleBrush.setStyle(Qt::SolidPattern);
    painter.setBrush(handleBrush);

    // Draw the outer handle
    handleBrush.setColor("#3fa9f5");
    painter.setBrush(handleBrush);
    painter.drawEllipse(rect.adjusted(-handlePen.width(), -handlePen.width(), handlePen.width()/2, handlePen.width()/2));

    // Draw the inner handle
    int adj = _settings.resizeHandleSize/2;
    handleBrush.setColor(Qt::white);
    painter.setBrush(handleBrush);
    painter.drawEllipse(rect.adjusted(-handlePen.width()+adj, -handlePen.width()+adj, (handlePen.width()/2)-adj, (handlePen.width()/2)-adj));
}
