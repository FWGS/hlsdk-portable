#include <VGUI.h>
#include <VGUI_App.h>
#include <VGUI_Color.h>
#include <VGUI_Cursor.h>
#include <VGUI_Dar.h>
#include <VGUI_KeyCode.h>
#include <VGUI_MouseCode.h>
#include <VGUI_Panel.h>
#include <VGUI_Scheme.h>

using namespace vgui;

App::App() {}
App::App(bool externalMain) {}
static App* App::getInstance() {return 0;}
void App::start() {}
void App::stop() {}
void App::externalTick() {}
bool App::wasMousePressed(MouseCode code,Panel* panel) {return false;}
bool App::wasMouseDoublePressed(MouseCode code,Panel* panel) {return false;}
bool App::isMouseDown(MouseCode code,Panel* panel) {return false;}
bool App::wasMouseReleased(MouseCode code,Panel* panel) {return false;}
bool App::wasKeyPressed(KeyCode code,Panel* panel) {return false;}
bool App::isKeyDown(KeyCode code,Panel* panel) {return false;}
bool App::wasKeyTyped(KeyCode code,Panel* panel) {return false;}
bool App::wasKeyReleased(KeyCode code,Panel* panel) {return false;}
void App::addTickSignal(TickSignal* s) {}
void App::setCursorPos(int x,int y) {}
void App::getCursorPos(int& x,int& y) {}
void App::setMouseCapture(Panel* panel) {}
void App::setMouseArena(int x0,int y0,int x1,int y1,bool enabled) {}
void App::setMouseArena(Panel* panel) {}
void App::requestFocus(Panel* panel) {}
Panel* App::getFocus() {return 0;}
void App::repaintAll() {}
void App::setScheme(Scheme* scheme) {}
Scheme* App::getScheme() {return 0;}
void App::enableBuildMode() {}
long App::getTimeMillis() {return 0;}
char App::getKeyCodeChar(KeyCode code,bool shifted) {return '\0';}
void App::getKeyCodeText(KeyCode code,char* buf,int buflen) {}
int App:: getClipboardTextCount() {return 0;}
void App::setClipboardText(const char* text,int textLen) {}
int App:: getClipboardText(int offset,char* buf,int bufLen) {return 0;}
void App::reset() {}
void App::internalSetMouseArena(int x0,int y0,int x1,int y1,bool enabled) {}
bool App::setRegistryString(const char* key,const char* value) {return false;}
bool App::getRegistryString(const char* key,char* value,int valueLen) {return false;}
bool App::setRegistryInteger(const char* key,int value) {return false;}
bool App::getRegistryInteger(const char* key,int& value) {return false;}
void App::setCursorOveride(Cursor* cursor) {}
Cursor* getCursorOveride() {return 0;}
void App::setMinimumTickMillisInterval(int interval) {}
void App::run() {}
void App::internalCursorMoved(int x,int y,SurfaceBase* surfaceBase) {}
void App::internalMousePressed(MouseCode code,SurfaceBase* surfaceBase) {}
void App::internalMouseDoublePressed(MouseCode code,SurfaceBase* surfaceBase) {}
void App::internalMouseReleased(MouseCode code,SurfaceBase* surfaceBase) {}
void App::internalMouseWheeled(int delta,SurfaceBase* surfaceBase) {}
void App::internalKeyPressed(KeyCode code,SurfaceBase* surfaceBase) {}
void App::internalKeyTyped(KeyCode code,SurfaceBase* surfaceBase) {}
void App::internalKeyReleased(KeyCode code,SurfaceBase* surfaceBase) {}
void App::init() {}
void App::updateMouseFocus(int x,int y,SurfaceBase* surfaceBase) {}
void App::setMouseFocus(Panel* newMouseFocus) {}
void App::surfaceBaseCreated(SurfaceBase* surfaceBase) {}
void App::surfaceBaseDeleted(SurfaceBase* surfaceBase) {}
void App::platTick() {}
void App::internalTick() {}

Color::Color() {}
Color::Color(int r,int g,int b,int a) {}
Color::Color(Scheme::SchemeColor sc) {}
void Color::init() {}
void Color::setColor(int r,int g,int b,int a) {}
void Color::setColor(Scheme::SchemeColor sc) {}
void Color::getColor(int& r,int& g,int& b,int& a) {}
void Color::getColor(Scheme::SchemeColor& sc) {}
int  Color::operator[](int index) {return 0;}

Cursor::Cursor(DefaultCursor dc) {}
Cursor::Cursor(Bitmap* bitmap,int hotspotX,int hotspotY) {}
void Cursor::getHotspot(int& x,int& y) {}
void Cursor::privateInit(Bitmap* bitmap,int hotspotX,int hotspotY) {}
Bitmap*       Cursor::getBitmap() {return 0;}
DefaultCursor Cursor::getDefaultCursor() {return dc_none;}

Panel::Panel() {}
Panel::Panel(int x,int y,int wide,int tall) {setPos(x, y); setSize(wide, tall);}
void init(int x,int y,int wide,int tall) {}
void Panel::setPos(int x,int y) {_pos[0] = x; _pos[1] = y;}
void Panel::getPos(int& x,int& y) {x = _pos[0]; y = _pos[1];}
void Panel::setSize(int wide,int tall) {_size[0] = wide, _size[1] = tall;}
void Panel::getSize(int& wide,int& tall) {wide = _size[0], tall = _size[1];}
void Panel::setBounds(int x,int y,int wide,int tall) {}
void Panel::getBounds(int& x,int& y,int& wide,int& tall) {}
int Panel::getWide() {return _size[0];}
int Panel::getTall() {return _size[1];}
Panel* Panel::getParent() {return _parent;}
void Panel::setVisible(bool state) {_visible = state;}
bool Panel::isVisible() {return _visible;}
bool Panel::isVisibleUp() {return false;}
void Panel::repaint() {}
void Panel::repaintAll() {}
void Panel::getAbsExtents(int& x0,int& y0,int& x1,int& y1) {}
void Panel::getClipRect(int& x0,int& y0,int& x1,int& y1) {}
void Panel::setParent(Panel* newParent) {_parent = newParent; newParent->addChild(this);}
void Panel::addChild(Panel* child) {}
void Panel::insertChildAt(Panel* child,int index) {}
void Panel::removeChild(Panel* child) {}
bool Panel::wasMousePressed(MouseCode code) {return false;}
bool Panel::wasMouseDoublePressed(MouseCode code) {return false;}
bool Panel::isMouseDown(MouseCode code) {return false;}
bool Panel::wasMouseReleased(MouseCode code) {return false;}
bool Panel::wasKeyPressed(KeyCode code) {return false;}
bool Panel::isKeyDown(KeyCode code) {return false;}
bool Panel::wasKeyTyped(KeyCode code) {return false;}
bool Panel::wasKeyReleased(KeyCode code) {return false;}
void Panel::addInputSignal(InputSignal* s) {}
void Panel::removeInputSignal(InputSignal* s) {}
void Panel::addRepaintSignal(RepaintSignal* s) {}
void Panel::removeRepaintSignal(RepaintSignal* s) {}
bool Panel::isWithin(int x,int y) {return false;} //in screen space
Panel* Panel::isWithinTraverse(int x,int y) {return 0;}
void Panel::localToScreen(int& x,int& y) {}
void Panel::screenToLocal(int& x,int& y) {}
void Panel::setCursor(Cursor* cursor) {}
void Panel::setCursor(Scheme::SchemeCursor scu) {}
Cursor* Panel::getCursor() {return 0;}
void Panel::setMinimumSize(int wide,int tall) {}
void Panel::getMinimumSize(int& wide,int& tall) {}
void Panel::requestFocus() {}
bool Panel::hasFocus() {return false;}
int Panel::getChildCount() {return 0;}
Panel* Panel::getChild(int index) {return 0;}
void Panel::setLayout(Layout* layout) {}
void Panel::invalidateLayout(bool layoutNow) {}
void Panel::setFocusNavGroup(FocusNavGroup* focusNavGroup) {}
void Panel::requestFocusPrev() {}
void Panel::requestFocusNext() {}
void Panel::addFocusChangeSignal(FocusChangeSignal* s) {}
bool Panel::isAutoFocusNavEnabled() {return false;}
void Panel::setAutoFocusNavEnabled(bool state) {}
void Panel::setBorder(Border* border) {}
void Panel::setPaintBorderEnabled(bool state) {}
void Panel::setPaintBackgroundEnabled(bool state) {}
void Panel::setPaintEnabled(bool state) {}
void Panel::getInset(int& left,int& top,int& right,int& bottom) {}
void Panel::getPaintSize(int& wide,int& tall) {}
void Panel::setPreferredSize(int wide,int tall) {}
void Panel::getPreferredSize(int& wide,int& tall) {}
SurfaceBase* Panel::getSurfaceBase() {return 0;}
bool Panel::isEnabled() {return _enabled = false;}
void Panel::setEnabled(bool state) {_enabled = true;}
void Panel::setBuildGroup(BuildGroup* buildGroup,const char* panelPersistanceName) {}
bool Panel::isBuildGroupEnabled() {return false;}
void Panel::removeAllChildren() {}
void Panel::repaintParent() {}
Panel* Panel::createPropertyPanel() {return 0;}
void Panel::getPersistanceText(char* buf,int bufLen) {}
void Panel::applyPersistanceText(const char* buf) {}
void Panel::setFgColor(Scheme::SchemeColor sc) {}
void Panel::setBgColor(Scheme::SchemeColor sc) {}
void Panel::setFgColor(int r,int g,int b,int a) {}
void Panel::setBgColor(int r,int g,int b,int a) {}
void Panel::getFgColor(int& r,int& g,int& b,int& a) {}
void Panel::getBgColor(int& r,int& g,int& b,int& a) {}
void Panel::setBgColor(Color color) {}
void Panel::setFgColor(Color color) {}
void Panel::getBgColor(Color& color) {}
void Panel::getFgColor(Color& color) {}
void Panel::setAsMouseCapture(bool state) {}
void Panel::setAsMouseArena(bool state) {}
App* Panel::getApp() {return 0;}
void Panel::getVirtualSize(int& wide,int& tall) {}
void Panel::setLayoutInfo(LayoutInfo* layoutInfo) {}
LayoutInfo* Panel::getLayoutInfo() {return 0;}
bool Panel::isCursorNone() {return false;}
void Panel::solveTraverse() {}
void Panel::paintTraverse() {}
void Panel::setSurfaceBaseTraverse(SurfaceBase* surfaceBase) {}
void Panel::performLayout() {}
void Panel::internalPerformLayout() {}
void Panel::drawSetColor(Scheme::SchemeColor sc) {}
void Panel::drawSetColor(int r,int g,int b,int a) {}
void Panel::drawFilledRect(int x0,int y0,int x1,int y1) {}
void Panel::drawOutlinedRect(int x0,int y0,int x1,int y1) {}
void Panel::drawSetTextFont(Scheme::SchemeFont sf) {}
void Panel::drawSetTextFont(Font* font) {}
void Panel::drawSetTextColor(Scheme::SchemeColor sc) {}
void Panel::drawSetTextColor(int r,int g,int b,int a) {}
void Panel::drawSetTextPos(int x,int y) {}
void Panel::drawPrintText(const char* str,int strlen) {}
void Panel::drawPrintText(int x,int y,const char* str,int strlen) {}
void Panel::drawPrintChar(char ch) {}
void Panel::drawPrintChar(int x,int y,char ch) {}
void Panel::drawSetTextureRGBA(int id,const char* rgba,int wide,int tall) {}
void Panel::drawSetTexture(int id) {}
void Panel::drawTexturedRect(int x0,int y0,int x1,int y1) {}
void Panel::solve() {}
void Panel::paintTraverse(bool repaint) {if(repaint) paintBackground();}
void Panel::paintBackground() {}
void Panel::paint() {}
void Panel::paintBuildOverlay() {}
void Panel::internalCursorMoved(int x,int y) {}
void Panel::internalCursorEntered() {}
void Panel::internalCursorExited() {}
void Panel::internalMousePressed(MouseCode code) {}
void Panel::internalMouseDoublePressed(MouseCode code) {}
void Panel::internalMouseReleased(MouseCode code) {}
void Panel::internalMouseWheeled(int delta) {}
void Panel::internalKeyPressed(KeyCode code) {}
void Panel::internalKeyTyped(KeyCode code) {}
void Panel::internalKeyReleased(KeyCode code) {}
void Panel::internalKeyFocusTicked() {}
void Panel::internalFocusChanged(bool lost) {}
void Panel::internalSetCursor() {}

Scheme::Scheme() {}
void Scheme::setColor(SchemeColor sc,int r,int g,int b,int a) {}
void Scheme::getColor(SchemeColor sc,int& r,int& g,int& b,int& a) {}
void Scheme::setFont(SchemeFont sf,Font* font) {}
Font* Scheme::getFont(SchemeFont sf) {return 0;}
void Scheme::setCursor(SchemeCursor sc,Cursor* cursor) {}
Cursor* Scheme::getCursor(SchemeCursor sc) {return 0;}
