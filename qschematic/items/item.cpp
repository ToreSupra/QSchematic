#include "item.hpp"
#include "../scene.hpp"
#include "../commands/item_move.hpp"

#include <QDebug>
#include <QPainter>
#include <QVector2D>
#include <QGraphicsSceneHoverEvent>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

using namespace QSchematic;
using namespace QSchematic::Items;

Item::Item(int type, QGraphicsItem* parent) :
    QGraphicsObject(parent),
    _type(type),
    _snapToGrid(true),
    _highlightEnabled(true),
    _highlighted(false),
    _oldRot{0}
{
    // Misc
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    connect(this, &Item::xChanged, this, &Item::posChanged);
    connect(this, &Item::yChanged, this, &Item::posChanged);
    connect(this, &Item::rotationChanged, this, &Item::rotChanged);

    // Connect signals from parent item
    Item* parentItem = static_cast<Item*>(parent);
    if (parentItem) {
        connect(parentItem, &Item::moved, this, &Item::scenePosChanged);
        connect(parentItem, &Item::rotated, this, &Item::scenePosChanged);
    }
}

Item::~Item()
{
    // Important insurance — if this is NOT expired, then we've been deleted
    // wrongly by Qt and all kinds of dirty shit will hit the fan!
    // Q_ASSERT(SharedPtrTracker::assert_expired(this));
    Q_ASSERT(weakPtr().expired());
}

template void Item::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive& ar, const unsigned int version);
template void Item::serialize<boost::archive::binary_iarchive>(boost::archive::binary_iarchive& ar, const unsigned int version);
template void Item::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive& ar, const unsigned int version);
template void Item::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive& ar, const unsigned int version);

template<class Archive>
void Item::save(Archive& ar, const unsigned int version) const
{
    Q_UNUSED(version)

    ar & boost::serialization::make_nvp("type", _type);
    qreal x = posX();
    ar & boost::serialization::make_nvp("x", x);
    qreal y = posY();
    ar & boost::serialization::make_nvp("y", y);
    qreal rot = this->rotation();
    ar & boost::serialization::make_nvp("rotation", rot);
    bool mov = isMovable();
    ar & boost::serialization::make_nvp("movable", mov);
    bool vis = isVisible();
    ar & boost::serialization::make_nvp("visible", vis);
    bool stg = this->snapToGrid();
    ar & boost::serialization::make_nvp("snap_to_grid", stg);
    bool he = this->highlightEnabled();
    ar & boost::serialization::make_nvp("highlight", he);
}

template<class Archive>
void Item::load(Archive& ar, const unsigned int version)
{
    Q_UNUSED(version)

    qreal x, y, rotation;
    bool movable, visible, snapToGrid, highlightEnabled;

    ar & boost::serialization::make_nvp("type", _type);
    ar & boost::serialization::make_nvp("x", x);
    ar & boost::serialization::make_nvp("y", y);
    ar & boost::serialization::make_nvp("rotation", rotation);
    ar & boost::serialization::make_nvp("movable", movable);
    ar & boost::serialization::make_nvp("visible", visible);
    ar & boost::serialization::make_nvp("snap_to_grid", snapToGrid);
    ar & boost::serialization::make_nvp("highlight", highlightEnabled);

    setPos(x, y);
    setRotation(rotation);
    setMovable(movable);
    setVisible(visible);
    setSnapToGrid(snapToGrid);
    setHighlightEnabled(highlightEnabled);
}

void Item::copyAttributes(Item& dest) const
{
    // Base class
    dest.setParentItem(parentItem());
    dest.setPos(pos());
    dest.setRotation(rotation());
    dest.setMovable(isMovable());
    dest.setVisible(isVisible());

    // Attributes
    dest._snapToGrid = _snapToGrid;
    dest._highlightEnabled = _highlightEnabled;
    dest._highlighted = _highlighted;
    dest._oldPos = _oldPos;
    dest._oldRot = _oldRot;
}

QSchematic::Scene* Item::scene() const
{
    return qobject_cast<QSchematic::Scene*>(QGraphicsObject::scene());
}

int Item::type() const
{
    return _type;
}

void Item::setGridPos(const QPoint& gridPos)
{
    setPos(_settings.toScenePoint(gridPos));
}

void Item::setGridPos(int x, int y)
{
    setGridPos(QPoint(x, y));
}

void Item::setGridPosX(int x)
{
    setGridPos(x, gridPosY());
}

void Item::setGridPosY(int y)
{
    setGridPos(gridPosX(), y);
}

QPoint Item::gridPos() const
{
    return _settings.toGridPoint(pos().toPoint());
}

int Item::gridPosX() const
{
    return gridPos().x();
}

int Item::gridPosY() const
{
    return gridPos().y();
}

void Item::setPos(const QPointF& pos)
{
    QGraphicsObject::setPos(pos);
}

void Item::setPos(qreal x, qreal y)
{
    QGraphicsObject::setPos(x, y);
}

void Item::setPosX(qreal x)
{
    setPos(x, posY());
}

void Item::setPosY(qreal y)
{
    setPos(posX(), y);
}

QPointF Item::pos() const
{
    return QGraphicsObject::pos();
}

qreal Item::posX() const
{
    return pos().x();
}

qreal Item::posY() const
{
    return pos().y();
}

void Item::setScenePos(const QPointF& point)
{
    QGraphicsObject::setPos(mapToParent(mapFromScene(point)));
}

void Item::setScenePos(qreal x, qreal y)
{
    setScenePos(QPointF(x, y));
}

void Item::setScenePosX(qreal x)
{
    setScenePos(x, scenePosY());
}

void Item::setScenePosY(qreal y)
{
    setScenePos(scenePosX(), y);
}

QPointF Item::scenePos() const
{
    return QGraphicsObject::scenePos();
}

qreal Item::scenePosX() const
{
    return scenePos().x();
}

qreal Item::scenePosY() const
{
    return scenePos().y();
}

void Item::moveBy(const QVector2D& moveBy)
{
    setPos(pos() + moveBy.toPointF());
}

void Item::setSettings(const Settings& settings)
{
    // Resnap to grid
    if (snapToGrid()) {
        setPos(_settings.snapToGrid(pos()));
    }

    // Store the new settings
    _settings = settings;

    // Let everyone know
    Q_EMIT settingsChanged();

    // Update
    update();
}

const Settings& Item::settings() const
{
    return _settings;
}

void Item::setMovable(bool enabled)
{
    setFlag(QGraphicsItem::ItemIsMovable, enabled);
}

bool Item::isMovable() const
{
    return flags() & QGraphicsItem::ItemIsMovable;
}

void Item::setSnapToGrid(bool enabled)
{
    _snapToGrid = enabled;
}

bool Item::snapToGrid() const
{
    return _snapToGrid;
}

bool Item::isHighlighted() const
{
    return ( ( _highlighted || isSelected() ) && _highlightEnabled );
}

void Item::setHighlighted(bool highlighted)
{
    _highlighted = highlighted;

    // Ripple through children
    for (QGraphicsItem* child : childItems()) {
        // Note: Using dynamic_cast instead of qgraphicsitem_cast to cope with user defined / custom items that might not
        //       be registered via QGraphicsItem tag based item conversion system.
        if (Item* childItem = dynamic_cast<Item*>(child); childItem)
            childItem->setHighlighted(highlighted);
    }
}

void Item::setHighlightEnabled(bool enabled)
{
    _highlightEnabled = enabled;
    _highlighted = false;
}

bool Item::highlightEnabled() const
{
    return _highlightEnabled;
}

QPixmap Item::toPixmap(QPointF& hotSpot, qreal scale)
{
    // Retrieve the bounding rect
    QRectF rectF = boundingRect();
    rectF = rectF.united(childrenBoundingRect());

    // Adjust the rectangle as the QPixmap doesn't handle negative coordinates
    rectF.setWidth(rectF.width() - rectF.x());
    rectF.setHeight(rectF.height() - rectF.y());
    const QRect& rect = rectF.toRect();
    if (rect.isNull() || !rect.isValid()) {
        return QPixmap();
    }

    // Provide the hot spot
    hotSpot = -rectF.topLeft();

    // Create the pixmap
    QPixmap pixmap(rect.size() * scale);
    pixmap.fill(Qt::transparent);

    // Render
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, _settings.antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing, _settings.antialiasing);
    painter.scale(scale, scale);
    painter.translate(hotSpot);
    QStyleOptionGraphicsItem option;
    paint(&painter, &option, nullptr);
    for (QGraphicsItem* child : childItems()) {
        if (!child)
            continue;
        
        painter.save();
        painter.translate(child->pos());
        child->paint(&painter, &option, nullptr);
        painter.restore();
    }

    painter.end();

    return pixmap;
}

QVariant Item::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    switch (change)
    {
    case QGraphicsItem::ItemSceneChange:
    {
        // NOTE: by doing this non-updated scene (ghost images staying) disappeared for some further cases (tips from usenet)
        prepareGeometryChange();
        return QGraphicsItem::itemChange(change, value);
    }
    case QGraphicsItem::ItemPositionChange:
    {
        QPointF newPos = value.toPointF();
        if (snapToGrid()) {
            newPos =_settings.snapToGrid(newPos);
        }
        return newPos;
    }
    case QGraphicsItem::ItemParentChange:
        if (parentObject()) {
            disconnect(parentObject(), nullptr, this, nullptr);
        }
        return value;
    case QGraphicsItem::ItemParentHasChanged:
    {
        Item* parent = static_cast<Item*>(parentItem());
        if (parent) {
            connect(parent, &Item::moved, this, &Item::scenePosChanged);
            connect(parent, &Item::rotated, this, &Item::scenePosChanged);
        }
        return value;
    }

    default:
        return QGraphicsItem::itemChange(change, value);
    }
}

void Item::posChanged()
{
    scenePosChanged();
    const QPointF& newPos = pos();
    QVector2D movedBy(newPos - _oldPos);
    if (!movedBy.isNull()) {
        Q_EMIT moved(*this, movedBy);
    }

    _oldPos = newPos;
}

void Item::scenePosChanged()
{
    Q_EMIT movedInScene(*this);
}

void Item::rotChanged()
{
    const qreal newRot = rotation();
    qreal rotationChange = newRot - _oldRot;
    if (!qFuzzyIsNull(rotationChange)) {
        Q_EMIT rotated(*this, rotationChange);
    }

    _oldRot = newRot;
}

void Item::update()
{
    // All transformations happen around the center of the item
    setTransformOriginPoint(boundingRect().width()/2, boundingRect().height()/2);

    // Base class
    QGraphicsObject::update();
}

