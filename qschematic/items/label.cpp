#include "label.hpp"
#include "../scene.hpp"

#include <QFontMetricsF>
#include <QPainter>
#include <QPen>
#include <QBrush>

#include <boost/serialization/nvp.hpp>

const QColor COLOR_LABEL             = QColor("#000000");
const QColor COLOR_LABEL_HIGHLIGHTED = QColor("#dc2479");
const qreal LABEL_TEXT_PADDING = 2;

BOOST_CLASS_EXPORT_IMPLEMENT(QSchematic::Items::Label)

using namespace QSchematic::Items;

Label::Label(int type, QGraphicsItem* parent) :
    Item(type, parent),
    _hasConnectionPoint(true)
{
    setSnapToGrid(false);
}

template void Label::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive& ar, const unsigned int version);
template void Label::serialize<boost::archive::binary_iarchive>(boost::archive::binary_iarchive& ar, const unsigned int version);
template void Label::serialize<boost::archive::xml_oarchive>(boost::archive::xml_oarchive& ar, const unsigned int version);
template void Label::serialize<boost::archive::xml_iarchive>(boost::archive::xml_iarchive& ar, const unsigned int version);

template<class Archive>
void Label::save(Archive& ar, const unsigned int version) const
{
    Q_UNUSED(version)

    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Item);
    std::string text = _text.toStdString();
    ar & boost::serialization::make_nvp("text", text);
    ar & boost::serialization::make_nvp("hasConnectionPoint", _hasConnectionPoint);
    qreal x = _connectionPoint.x();
    ar & boost::serialization::make_nvp("connectionPoint_x", x);
    qreal y = _connectionPoint.y();
    ar & boost::serialization::make_nvp("connectionPoint_y", y);
}

template<class Archive>
void Label::load(Archive& ar, const unsigned int version)
{
    Q_UNUSED(version)

    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Item);
    std::string text;
    ar & boost::serialization::make_nvp("text", text);
    setText(QString::fromStdString(text));
    ar & boost::serialization::make_nvp("hasConnectionPoint", _hasConnectionPoint);
    qreal x, y;
    ar & boost::serialization::make_nvp("connectionPoint_x", x);
    ar & boost::serialization::make_nvp("connectionPoint_y", y);
    setConnectionPoint(QPointF(x, y));
}

std::shared_ptr<Item> Label::deepCopy() const
{
    auto clone = std::make_shared<Label>(type(), parentItem());
    copyAttributes(*clone);

    return clone;
}

void Label::copyAttributes(Label& dest) const
{
    // Base class
    Item::copyAttributes(dest);

    // Attributes
    dest._text = _text;
    dest._font = _font;
    dest._textRect = _textRect;
    dest._hasConnectionPoint = _hasConnectionPoint;
    dest._connectionPoint = _connectionPoint;
}

QRectF Label::boundingRect() const
{
    QRectF rect = _textRect;
    if(isHighlighted()) {
        rect = rect.united(QRectF(_textRect.center(), mapFromParent(_connectionPoint)));
    }
    return rect;
}

QPainterPath Label::shape() const
{
    QPainterPath path;
    path.addRect(_textRect);
    return path;
}

void Label::setText(const QString& text)
{
    _text = text;
    calculateTextRect();
    Q_EMIT textChanged(_text);
}

void Label::setFont(const QFont& font)
{
    _font = font;

    calculateTextRect();
}

void Label::setHasConnectionPoint(bool enabled)
{
    _hasConnectionPoint = enabled;
}

bool Label::hasConnectionPoint() const
{
    return _hasConnectionPoint;
}

void Label::setConnectionPoint(const QPointF& connectionPoint)
{
    _connectionPoint = connectionPoint;

    Item::update();
}

void Label::calculateTextRect()
{
    QFontMetricsF fontMetrics(_font);
    _textRect = fontMetrics.boundingRect(_text);
    _textRect.adjust(-LABEL_TEXT_PADDING, -LABEL_TEXT_PADDING, LABEL_TEXT_PADDING, LABEL_TEXT_PADDING);
}

QString Label::text() const
{
    return _text;
}

QFont Label::font() const
{
    return _font;
}

QRectF Label::textRect() const
{
    return _textRect;
}

void Label::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    // Draw a dashed line to the wire if selected
    if (isHighlighted()) {
        // Line pen
        QPen penLine;
        penLine.setColor(COLOR_LABEL_HIGHLIGHTED);
        penLine.setStyle(Qt::DashLine);

        // Line brush
        QBrush brushLine;
        brushLine.setStyle(Qt::NoBrush);

        // Draw the connection line
        if (_hasConnectionPoint) {
            painter->setPen(penLine);
            painter->setBrush(brushLine);
            painter->drawLine(_textRect.center(), mapFromParent(_connectionPoint));
        }

        // Clear the text rectangle
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::white);
        painter->drawRect(_textRect.adjusted(penLine.width()/2, penLine.width()/2, -penLine.width()/2, -penLine.width()/2));

        // Draw the border around the label text
        painter->setPen(penLine);
        painter->setBrush(brushLine);
        painter->drawRect(_textRect);
    }

    // Text pen
    QPen textPen;
    textPen.setStyle(Qt::SolidLine);
    textPen.setColor(Qt::black);

    // Text option
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::NoWrap);
    textOption.setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    // Draw the text
    painter->setPen(COLOR_LABEL);
    painter->setBrush(Qt::NoBrush);
    painter->setFont(_font);
    painter->drawText(_textRect, _text, textOption);

    // Draw the bounding rect if debug mode is enabled
    if (_settings.debug) {
        painter->setPen(Qt::red);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
        painter->setPen(Qt::blue);
        painter->drawPath(shape());
    }
}

void Label::mouseDoubleClickEvent([[maybe_unused]] QGraphicsSceneMouseEvent* event)
{
    Q_EMIT doubleClicked();
}
