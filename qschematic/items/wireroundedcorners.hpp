#pragma once

#include "wire.hpp"

namespace QSchematic::Items
{

    class WireRoundedCorners :
        public Wire
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(WireRoundedCorners)

    public:
        WireRoundedCorners(int type = Item::WireRoundedCornersType, QGraphicsItem* parent = nullptr);
        ~WireRoundedCorners() override = default;

        friend class ::boost::serialization::access;
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version);
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    private:
        enum QuarterCircleSegment {
            None,
            TopLeft,
            TopRight,
            BottomLeft,
            BottomRight
        };
    };

}

BOOST_CLASS_EXPORT_KEY(QSchematic::Items::WireRoundedCorners)
