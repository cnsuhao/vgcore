﻿// mgdrawarc.cpp
// License: LGPL, https://github.com/rhcad/touchvg

#include "mgdrawarc.h"
#include "mgbasicsps.h"
#include "mgstorage.h"
#include "mglocal.h"
#include <sstream>

// MgCmdArc3P
//

bool MgCmdArc3P::initialize(const MgMotion* sender, MgStorage* s)
{
    for (int i = 0; i < 3; i++)
        _points[i] = Point2d();
    return _initialize(MgArc::Type(), sender, s);
}

void MgCmdArc3P::drawArcHandle(const MgMotion* sender, GiGraphics* gs)
{
    if (m_step > 0 && m_step < 3
        && sender->dragging() && !getSnappedType(sender)) {
        gs->drawHandle(_points[m_step], kGiHandleHotVertex);
    }
}

bool MgCmdArc3P::draw(const MgMotion* sender, GiGraphics* gs)
{
    drawArcHandle(sender, gs);
    if (m_step > 0) {
        GiContext ctx(-2, GiColor(0, 126, 0, 32), GiContext::kDotLine);
        gs->drawLine(&ctx, _points[0], _points[1]);
        
        MgArc* arc = (MgArc*)dynshape()->shape();
        gs->drawCircle(&ctx, arc->getCenter(), arc->getRadius());
    }
    return MgCommandDraw::draw(sender, gs);
}

void MgCmdArc3P::setStepPoint(const MgMotion*, int step, const Point2d& pt)
{
    MgArc* arc = (MgArc*)dynshape()->shape();

    if (step == 0) {
        _points[0] = pt;
    }
    else if (step == 1) {
        _points[1] = pt;
        _points[2] = pt;
        arc->setStartMidEnd(_points[0], pt, pt);
    }
    else if (step == 2) {
        _points[2] = pt;
        arc->setStartMidEnd(_points[0], _points[1], pt);
    }
}

bool MgCmdArcCSE::initialize(const MgMotion* sender, MgStorage* s)
{
    if (s) {
        _decimal = s->readInt("decimal", _decimal);
    }
    return MgCmdArc3P::initialize(sender, s);
}

float MgCommand::drawAngleText(const MgMotion* sender, GiGraphics* gs, float angle, void* stdstr)
{
    std::stringstream ss;
    
    Point2d pt(sender->pointM + Vector2d(0, sender->displayMmToModel(12.f)));
    pt.y = mgMin(pt.y, sender->view->xform()->getWndRectM().ymax);
    
    ss << mgbase::roundReal(mgbase::rad2Deg(angle), 2) << MgLocalized::getString(sender->view, "degrees");
    if (stdstr) {
        *((std::string*)stdstr) = ss.str();
    }
    return gs ? gs->drawTextAt(GiColor::Red().getARGB(), ss.str().c_str(), pt, 3.5f) : 0.f;
}

bool MgCmdArcCSE::draw(const MgMotion* sender, GiGraphics* gs)
{
    if (m_step == 2 && sender->dragging()) {    // 画弧时显示圆心与终端连线
        GiContext ctx(0, GiColor(0, 126, 0, 64), GiContext::kDotLine);
        gs->drawLine(&ctx, _points[0], _points[2]);
        drawAngleText(sender, gs, fabsf(((MgArc*)dynshape()->shape())->getSweepAngle()));
    }
    if (_points[0] != _points[1]) {
        gs->drawHandle(_points[0], kGiHandleCenter);
        if (m_step == 0) {
            GiContext ctx(-2, GiColor(0, 126, 0, 32), GiContext::kDashLine);
            gs->drawCircle(&ctx, _points[0], _points[0].distanceTo(_points[1]));
        }
    }
    return MgCmdArc3P::draw(sender, gs);
}

bool MgCmdArcCSE::click(const MgMotion* sender)
{
    Point2d pt(snapPoint(sender));
    
    _points[1] += pt - _points[0];  // 半径不变
    _points[2] += pt - _points[0];  // 半径不变
    _points[0] = pt;                // 定圆心
    sender->view->redraw();
    
    return true;
}

void MgCmdArcCSE::setStepPoint(const MgMotion*, int step, const Point2d& pt)
{
    MgArc* arc = (MgArc*)dynshape()->shape();

    if (step == 0) {
        if (_points[1] == _points[2]) {
            _points[0] = pt;                // 记下圆心
            arc->offset(pt - arc->getCenter(), -1);
        } else {                            // 设置起始方向
            float r = _points[0].distanceTo(_points[2]);
            _points[1] = _points[0].rulerPoint(pt, r, 0);
            if (_points[1] == _points[2]) {
                _points[2] = _points[0].rulerPoint(pt, -r, 0);  // 保持不同
            }
            arc->setCenterStartEnd(_points[0], _points[1]); // 初始转角为0
            m_step = 2;
        }
    }
    else if (step == 1) {
        _points[1] = pt;            // 记下起点
        _points[2] = pt;            // 起点与终点重合
        arc->setCenterStartEnd(_points[0], _points[1]); // 初始转角为0
    }
    else if (step == 2) {
        arc->setCenterStartEnd(_points[0], _points[1], pt);
        float angle = mgbase::roundReal(arc->getSweepAngle() * _M_R2D, _decimal) * _M_D2R;
        arc->setCenterRadius(arc->getCenter(), arc->getRadius(), arc->getStartAngle(), angle);
        _points[2] = arc->getEndPoint();    // 记下终点
    }
}

void MgCmdArcTan::setStepPoint(const MgMotion*, int step, const Point2d& pt)
{
    MgArc* arc = (MgArc*)dynshape()->shape();

    if (step == 0) {
        _points[0] = pt;
    }
    else if (step == 1) {
        _points[1] = pt;
        _points[2] = pt;
        arc->setTanStartEnd(_points[1] - _points[0], pt, pt);
    }
    else if (step == 2) {
        arc->setTanStartEnd(_points[1] - _points[0], _points[1], pt);
        _points[2] = pt;
    }
}
