#!/usr/bin/env python3
"""
U8g2 Screen Designer — SH1106 128×64 OLED Screen Editor
A PyQt6 tool for designing and generating U8g2 Arduino display code.
"""

import sys
import json
import math
from enum import Enum
from dataclasses import dataclass, field, asdict
from typing import List, Optional, Tuple
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QGridLayout, QLabel, QPushButton, QButtonGroup, QTextEdit,
    QListWidget, QListWidgetItem, QSplitter, QInputDialog, QMessageBox,
    QColorDialog, QSpinBox, QComboBox, QFrame, QScrollArea,
    QSizePolicy, QToolTip, QSlider, QCheckBox, QFileDialog, QGroupBox,
    QLineEdit, QDialog, QDialogButtonBox, QFontComboBox
)
from PyQt6.QtCore import (
    Qt, QRect, QPoint, QSize, pyqtSignal, QTimer, QRectF, QPointF
)
from PyQt6.QtGui import (
    QPainter, QPen, QBrush, QColor, QPixmap, QFont, QFontMetrics,
    QPainterPath, QPolygon, QKeySequence, QShortcut, QIcon, QCursor,
    QPalette, QLinearGradient, QRadialGradient
)


# ─── DISPLAY CONSTANTS ────────────────────────────────────────────────────────
DISPLAY_W = 128
DISPLAY_H = 64
PIXEL_SIZE = 7          # each OLED pixel = 7×7 screen pixels
CANVAS_W = DISPLAY_W * PIXEL_SIZE
CANVAS_H = DISPLAY_H * PIXEL_SIZE

# ─── TOOL MODES ───────────────────────────────────────────────────────────────
class Tool(Enum):
    SELECT    = "select"
    MOVE      = "move"
    PEN       = "pen"
    LINE      = "line"
    RECT      = "rect"
    RECT_FILL = "rect_fill"
    CIRCLE    = "circle"
    CIRCLE_FILL = "circle_fill"
    TEXT      = "text"
    ERASER    = "eraser"

# ─── SHAPE DATA CLASSES ───────────────────────────────────────────────────────
@dataclass
class Shape:
    kind: str
    color: int = 1   # 1 = white (on), 0 = black (off)

@dataclass
class PixelShape(Shape):
    kind: str = "pixel"
    x: int = 0
    y: int = 0

@dataclass
class LineShape(Shape):
    kind: str = "line"
    x0: int = 0
    y0: int = 0
    x1: int = 0
    y1: int = 0

@dataclass
class RectShape(Shape):
    kind: str = "rect"
    x: int = 0
    y: int = 0
    w: int = 0
    h: int = 0
    filled: bool = False

@dataclass
class CircleShape(Shape):
    kind: str = "circle"
    cx: int = 0
    cy: int = 0
    r: int = 0
    filled: bool = False

@dataclass
class TextShape(Shape):
    kind: str = "text"
    x: int = 0
    y: int = 0
    text: str = ""
    font: str = "u8g2_font_ncenB08_tr"

# ─── SCREEN DATA ──────────────────────────────────────────────────────────────
@dataclass
class Screen:
    name: str = "Screen"
    shapes: List[dict] = field(default_factory=list)

    def to_dict(self):
        return {"name": self.name, "shapes": self.shapes}

    @staticmethod
    def from_dict(d):
        return Screen(name=d["name"], shapes=d.get("shapes", []))


# ─── U8G2 FONT LIST ───────────────────────────────────────────────────────────
U8G2_FONTS = [
    "u8g2_font_4x6_tr",
    "u8g2_font_5x7_tr",
    "u8g2_font_5x8_tr",
    "u8g2_font_6x10_tr",
    "u8g2_font_6x12_tr",
    "u8g2_font_7x13_tr",
    "u8g2_font_8x13_tr",
    "u8g2_font_ncenB08_tr",
    "u8g2_font_ncenB10_tr",
    "u8g2_font_ncenB12_tr",
    "u8g2_font_ncenB14_tr",
    "u8g2_font_helvB08_tr",
    "u8g2_font_helvB10_tr",
    "u8g2_font_helvB12_tr",
    "u8g2_font_profont10_tr",
    "u8g2_font_profont12_tr",
    "u8g2_font_profont15_tr",
    "u8g2_font_profont17_tr",
    "u8g2_font_unifont_t_latin",
    "u8g2_font_micro_tr",
    "u8g2_font_tom_thumb_4x6_tr",
    "u8g2_font_squeezed_r7_tr",
]

# Approximate font heights for preview rendering
FONT_HEIGHTS = {
    "u8g2_font_4x6_tr": 6,
    "u8g2_font_5x7_tr": 7,
    "u8g2_font_5x8_tr": 8,
    "u8g2_font_6x10_tr": 10,
    "u8g2_font_6x12_tr": 12,
    "u8g2_font_7x13_tr": 13,
    "u8g2_font_8x13_tr": 13,
    "u8g2_font_ncenB08_tr": 8,
    "u8g2_font_ncenB10_tr": 10,
    "u8g2_font_ncenB12_tr": 12,
    "u8g2_font_ncenB14_tr": 14,
    "u8g2_font_helvB08_tr": 8,
    "u8g2_font_helvB10_tr": 10,
    "u8g2_font_helvB12_tr": 12,
    "u8g2_font_profont10_tr": 10,
    "u8g2_font_profont12_tr": 12,
    "u8g2_font_profont15_tr": 15,
    "u8g2_font_profont17_tr": 17,
    "u8g2_font_unifont_t_latin": 16,
    "u8g2_font_micro_tr": 5,
    "u8g2_font_tom_thumb_4x6_tr": 6,
    "u8g2_font_squeezed_r7_tr": 7,
}


# ─── CODE GENERATOR ────────────────────────────────────────────────────────────────────────────
def generate_code(shapes, func_name="drawScreen"):
    """Emit U8g2 C code for the current visible snapshot only.

    shapes is an append-only edit log. Emitting it verbatim causes duplicated
    drawPixel calls (same coord drawn multiple times), stale pixels surviving
    after erasing, and old positions persisting after a move.

    We fix this by:
      - Collapsing pixel-kind entries to a coord->color map (last write wins).
      - Keeping high-level shapes (rect/circle/line/text) in draw order.
      - Rasterising those shapes to skip redundant drawPixel calls.
    """
    pixel_map = {}   # (x,y) -> color  — last write wins
    high_level = []  # rect / circle / line / text in draw order

    for s in shapes:
        k = s.get("kind")
        if k == "pixel":
            pixel_map[(s["x"], s["y"])] = s.get("color", 1)
        elif k in ("rect", "circle", "line", "text"):
            high_level.append(s)

    # Rasterise high-level shapes so we know which pixels they already cover
    covered = set()

    def _bres(x0, y0, x1, y1):
        pts = []
        dx = abs(x1-x0); sx = 1 if x0 < x1 else -1
        dy = -abs(y1-y0); sy = 1 if y0 < y1 else -1
        err = dx + dy
        while True:
            pts.append((x0, y0))
            if x0 == x1 and y0 == y1: break
            e2 = 2*err
            if e2 >= dy: err += dy; x0 += sx
            if e2 <= dx: err += dx; y0 += sy
        return pts

    for s in high_level:
        k = s.get("kind")
        if k == "line":
            for pt in _bres(s["x0"], s["y0"], s["x1"], s["y1"]):
                covered.add(pt)
        elif k == "rect":
            x, y, w, h = s["x"], s["y"], s["w"], s["h"]
            if s.get("filled"):
                for dy_ in range(h):
                    for dx_ in range(w):
                        covered.add((x+dx_, y+dy_))
            else:
                for pt in (_bres(x,y,x+w-1,y) + _bres(x+w-1,y,x+w-1,y+h-1) +
                           _bres(x+w-1,y+h-1,x,y+h-1) + _bres(x,y+h-1,x,y)):
                    covered.add(pt)
        elif k == "circle":
            cx, cy, r = s["cx"], s["cy"], s["r"]
            if s.get("filled"):
                for dy_ in range(-r, r+1):
                    for dx_ in range(-r, r+1):
                        if dx_*dx_ + dy_*dy_ <= r*r:
                            covered.add((cx+dx_, cy+dy_))
            else:
                xi, yi, err = r, 0, 0
                while xi >= yi:
                    for pt in [(cx+xi,cy+yi),(cx-xi,cy+yi),(cx+xi,cy-yi),(cx-xi,cy-yi),
                               (cx+yi,cy+xi),(cx-yi,cy+xi),(cx+yi,cy-xi),(cx-yi,cy-xi)]:
                        covered.add(pt)
                    yi += 1; err += 2*yi+1
                    if 2*(err-xi)+1 > 0: xi -= 1; err += 2*(1-xi)
        elif k == "text":
            fh = FONT_HEIGHTS.get(s.get("font", ""), 8)
            scale = max(1, round(fh / 7))
            cx = s["x"]
            for ch in s.get("text", ""):
                code = ord(ch)
                if 32 <= code <= 126:
                    idx = (code - 32) * 5
                    for col in range(5):
                        col_data = _FONT5x7_RAW[idx + col]
                        for row in range(7):
                            if col_data & (1 << row):
                                for sy_ in range(scale):
                                    for sx_ in range(scale):
                                        covered.add((cx + col*scale + sx_,
                                                     s["y"] - (6-row)*scale - sy_))
                cx += 5*scale + scale  # char_w + spacing

    # Build output lines
    out = []
    out.append("void " + func_name + "() {")
    out.append("  u8g2.clearBuffer();")
    out.append("  u8g2.setDrawColor(1);")
    out.append("")

    cur_color = [1]
    cur_font  = [None]

    def set_color(c):
        if c != cur_color[0]:
            out.append("  u8g2.setDrawColor(" + str(c) + ");")
            cur_color[0] = c

    for s in high_level:
        k = s.get("kind")
        c = s.get("color", 1)
        set_color(c)
        if k == "line":
            out.append("  u8g2.drawLine(" + str(s["x0"]) + ", " + str(s["y0"]) +
                       ", " + str(s["x1"]) + ", " + str(s["y1"]) + ");")
        elif k == "rect":
            if s.get("filled"):
                out.append("  u8g2.drawBox(" + str(s["x"]) + ", " + str(s["y"]) +
                           ", " + str(s["w"]) + ", " + str(s["h"]) + ");")
            else:
                out.append("  u8g2.drawFrame(" + str(s["x"]) + ", " + str(s["y"]) +
                           ", " + str(s["w"]) + ", " + str(s["h"]) + ");")
        elif k == "circle":
            if s.get("filled"):
                out.append("  u8g2.drawDisc(" + str(s["cx"]) + ", " + str(s["cy"]) +
                           ", " + str(s["r"]) + ");")
            else:
                out.append("  u8g2.drawCircle(" + str(s["cx"]) + ", " + str(s["cy"]) +
                           ", " + str(s["r"]) + ");")
        elif k == "text":
            font = s.get("font", "u8g2_font_ncenB08_tr")
            if font != cur_font[0]:
                out.append("  u8g2.setFont(" + font + ");")
                cur_font[0] = font
            txt = s.get("text", "").replace('"', '\\"')
            out.append('  u8g2.drawStr(' + str(s["x"]) + ', ' + str(s["y"]) +
                       ', "' + txt + '");')

    on_px = sorted(
        [(x, y) for (x, y), c in pixel_map.items() if c and (x, y) not in covered],
        key=lambda p: (p[1], p[0])
    )
    off_px = sorted(
        [(x, y) for (x, y), c in pixel_map.items() if not c and (x, y) in covered],
        key=lambda p: (p[1], p[0])
    )

    if on_px:
        set_color(1)
        for x, y in on_px:
            out.append("  u8g2.drawPixel(" + str(x) + ", " + str(y) + ");")

    if off_px:
        set_color(0)
        for x, y in off_px:
            out.append("  u8g2.drawPixel(" + str(x) + ", " + str(y) + ");")

    out.append("")
    out.append("  u8g2.sendBuffer();")
    out.append("}")
    return "\n".join(out)


# ─── PIXEL FONT DATA ──────────────────────────────────────────────────────────
# Compact 5×7 pixel font bitmaps (printable ASCII 32–126).
# Each char is 5 columns of 7 bits, stored as a list of 5 ints (top bit = row 0).
_FONT5x7_RAW = (
    0x00,0x00,0x00,0x00,0x00, # 32 space
    0x00,0x00,0x5F,0x00,0x00, # 33 !
    0x00,0x07,0x00,0x07,0x00, # 34 "
    0x14,0x7F,0x14,0x7F,0x14, # 35 #
    0x24,0x2A,0x7F,0x2A,0x12, # 36 $
    0x23,0x13,0x08,0x64,0x62, # 37 %
    0x36,0x49,0x55,0x22,0x50, # 38 &
    0x00,0x05,0x03,0x00,0x00, # 39 '
    0x00,0x1C,0x22,0x41,0x00, # 40 (
    0x00,0x41,0x22,0x1C,0x00, # 41 )
    0x14,0x08,0x3E,0x08,0x14, # 42 *
    0x08,0x08,0x3E,0x08,0x08, # 43 +
    0x00,0x50,0x30,0x00,0x00, # 44 ,
    0x08,0x08,0x08,0x08,0x08, # 45 -
    0x00,0x60,0x60,0x00,0x00, # 46 .
    0x20,0x10,0x08,0x04,0x02, # 47 /
    0x3E,0x51,0x49,0x45,0x3E, # 48 0
    0x00,0x42,0x7F,0x40,0x00, # 49 1
    0x42,0x61,0x51,0x49,0x46, # 50 2
    0x21,0x41,0x45,0x4B,0x31, # 51 3
    0x18,0x14,0x12,0x7F,0x10, # 52 4
    0x27,0x45,0x45,0x45,0x39, # 53 5
    0x3C,0x4A,0x49,0x49,0x30, # 54 6
    0x01,0x71,0x09,0x05,0x03, # 55 7
    0x36,0x49,0x49,0x49,0x36, # 56 8
    0x06,0x49,0x49,0x29,0x1E, # 57 9
    0x00,0x36,0x36,0x00,0x00, # 58 :
    0x00,0x56,0x36,0x00,0x00, # 59 ;
    0x08,0x14,0x22,0x41,0x00, # 60 <
    0x14,0x14,0x14,0x14,0x14, # 61 =
    0x00,0x41,0x22,0x14,0x08, # 62 >
    0x02,0x01,0x51,0x09,0x06, # 63 ?
    0x32,0x49,0x79,0x41,0x3E, # 64 @
    0x7E,0x11,0x11,0x11,0x7E, # 65 A
    0x7F,0x49,0x49,0x49,0x36, # 66 B
    0x3E,0x41,0x41,0x41,0x22, # 67 C
    0x7F,0x41,0x41,0x22,0x1C, # 68 D
    0x7F,0x49,0x49,0x49,0x41, # 69 E
    0x7F,0x09,0x09,0x09,0x01, # 70 F
    0x3E,0x41,0x49,0x49,0x7A, # 71 G
    0x7F,0x08,0x08,0x08,0x7F, # 72 H
    0x00,0x41,0x7F,0x41,0x00, # 73 I
    0x20,0x40,0x41,0x3F,0x01, # 74 J
    0x7F,0x08,0x14,0x22,0x41, # 75 K
    0x7F,0x40,0x40,0x40,0x40, # 76 L
    0x7F,0x02,0x0C,0x02,0x7F, # 77 M
    0x7F,0x04,0x08,0x10,0x7F, # 78 N
    0x3E,0x41,0x41,0x41,0x3E, # 79 O
    0x7F,0x09,0x09,0x09,0x06, # 80 P
    0x3E,0x41,0x51,0x21,0x5E, # 81 Q
    0x7F,0x09,0x19,0x29,0x46, # 82 R
    0x46,0x49,0x49,0x49,0x31, # 83 S
    0x01,0x01,0x7F,0x01,0x01, # 84 T
    0x3F,0x40,0x40,0x40,0x3F, # 85 U
    0x1F,0x20,0x40,0x20,0x1F, # 86 V
    0x3F,0x40,0x38,0x40,0x3F, # 87 W
    0x63,0x14,0x08,0x14,0x63, # 88 X
    0x07,0x08,0x70,0x08,0x07, # 89 Y
    0x61,0x51,0x49,0x45,0x43, # 90 Z
    0x00,0x7F,0x41,0x41,0x00, # 91 [
    0x02,0x04,0x08,0x10,0x20, # 92 backslash
    0x00,0x41,0x41,0x7F,0x00, # 93 ]
    0x04,0x02,0x01,0x02,0x04, # 94 ^
    0x40,0x40,0x40,0x40,0x40, # 95 _
    0x00,0x01,0x02,0x04,0x00, # 96 `
    0x20,0x54,0x54,0x54,0x78, # 97 a
    0x7F,0x48,0x44,0x44,0x38, # 98 b
    0x38,0x44,0x44,0x44,0x20, # 99 c
    0x38,0x44,0x44,0x48,0x7F, # 100 d
    0x38,0x54,0x54,0x54,0x18, # 101 e
    0x08,0x7E,0x09,0x01,0x02, # 102 f
    0x0C,0x52,0x52,0x52,0x3E, # 103 g
    0x7F,0x08,0x04,0x04,0x78, # 104 h
    0x00,0x44,0x7D,0x40,0x00, # 105 i
    0x20,0x40,0x44,0x3D,0x00, # 106 j
    0x7F,0x10,0x28,0x44,0x00, # 107 k
    0x00,0x41,0x7F,0x40,0x00, # 108 l
    0x7C,0x04,0x18,0x04,0x78, # 109 m
    0x7C,0x08,0x04,0x04,0x78, # 110 n
    0x38,0x44,0x44,0x44,0x38, # 111 o
    0x7C,0x14,0x14,0x14,0x08, # 112 p
    0x08,0x14,0x14,0x18,0x7C, # 113 q
    0x7C,0x08,0x04,0x04,0x08, # 114 r
    0x48,0x54,0x54,0x54,0x20, # 115 s
    0x04,0x3F,0x44,0x40,0x20, # 116 t
    0x3C,0x40,0x40,0x20,0x7C, # 117 u
    0x1C,0x20,0x40,0x20,0x1C, # 118 v
    0x3C,0x40,0x30,0x40,0x3C, # 119 w
    0x44,0x28,0x10,0x28,0x44, # 120 x
    0x0C,0x50,0x50,0x50,0x3C, # 121 y
    0x44,0x64,0x54,0x4C,0x44, # 122 z
    0x00,0x08,0x36,0x41,0x00, # 123 {
    0x00,0x00,0x7F,0x00,0x00, # 124 |
    0x00,0x41,0x36,0x08,0x00, # 125 }
    0x10,0x08,0x08,0x10,0x08, # 126 ~
)

def _render_char_5x7(pixels, char_code, x_off, y_off, color):
    """Render a single 5x7 character into the pixel buffer. y_off is the baseline (bottom of char)."""
    idx = (char_code - 32) * 5
    if idx < 0 or idx + 5 > len(_FONT5x7_RAW):
        return
    for col in range(5):
        col_data = _FONT5x7_RAW[idx + col]
        for row in range(7):
            if col_data & (1 << row):
                px = x_off + col
                py = y_off - 6 + row   # baseline offset: row 0 is top of glyph
                if 0 <= px < DISPLAY_W and 0 <= py < DISPLAY_H:
                    pixels[py][px] = color

def render_text_to_buffer(pixels, x, y, text, font, color):
    """Render text into the pixel buffer using the built-in 5x7 font.

    x, y follow U8g2 convention: y is the BASELINE (bottom of capital letters).
    The glyph occupies rows (y - ascent + 1) .. y where ascent = 6*scale for
    our scaled 5x7 font (7 rows, row 6 is at the baseline).

    Row encoding: bit 0 = row 0 (TOP of glyph), bit 6 = row 6 (BASELINE row).
    So pixel row = y - (6 - row) * scale.
    """
    fh = FONT_HEIGHTS.get(font, 8)
    scale = max(1, round(fh / 7))
    char_w = 5 * scale
    spacing = scale

    cx = x
    for ch in text:
        code = ord(ch)
        if 32 <= code <= 126:
            idx = (code - 32) * 5
            for col in range(5):
                col_data = _FONT5x7_RAW[idx + col]
                for row in range(7):
                    if col_data & (1 << row):
                        for sy in range(scale):
                            for sx in range(scale):
                                px = cx + col * scale + sx
                                # base_py is the TOP pixel of this glyph row.
                                # Subtract (scale-1) so the BOTTOM of row 6 = y.
                                py = y - (6 - row) * scale - (scale - 1) + sy
                                if 0 <= px < DISPLAY_W and 0 <= py < DISPLAY_H:
                                    pixels[py][px] = color
        cx += char_w + spacing
    return cx


# ─── CANVAS WIDGET ────────────────────────────────────────────────────────────
class OLEDCanvas(QWidget):
    shapes_changed = pyqtSignal()
    coord_changed = pyqtSignal(int, int)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedSize(CANVAS_W, CANVAS_H)
        self.setCursor(Qt.CursorShape.CrossCursor)
        self.setMouseTracking(True)

        self.shapes: List[dict] = []
        self.undo_stack: List[List[dict]] = []
        self.redo_stack: List[List[dict]] = []

        self.tool = Tool.PEN
        self.draw_color = 1
        self.selected_font = 'u8g2_font_5x7_tr'  # matches built-in preview font

        # Drawing state
        self._drawing = False
        self._start: Optional[QPoint] = None
        self._current: Optional[QPoint] = None
        self._pen_pixels: List[dict] = []

        # Selection / move state
        self._sel_rect: Optional[QRect] = None          # in OLED coords
        self._sel_pixels: List[Tuple[int,int]] = []     # selected lit pixel positions
        self._moving = False
        self._move_origin: Optional[Tuple[int,int]] = None  # oled coords at drag start
        self._move_ghost: List[Tuple[int,int]] = []         # pixel positions being dragged

        self.show_grid = True

    # ── coordinate helpers ──
    def px_to_oled(self, qp: QPoint) -> Tuple[int, int]:
        x = max(0, min(DISPLAY_W - 1, qp.x() // PIXEL_SIZE))
        y = max(0, min(DISPLAY_H - 1, qp.y() // PIXEL_SIZE))
        return x, y

    # ── undo/redo ──
    def _push_undo(self):
        import copy
        self.undo_stack.append(copy.deepcopy(self.shapes))
        self.redo_stack.clear()
        if len(self.undo_stack) > 50:
            self.undo_stack.pop(0)

    def undo(self):
        if self.undo_stack:
            import copy
            self.redo_stack.append(copy.deepcopy(self.shapes))
            self.shapes = self.undo_stack.pop()
            self._clear_selection()
            self.shapes_changed.emit()
            self.update()

    def redo(self):
        if self.redo_stack:
            import copy
            self.undo_stack.append(copy.deepcopy(self.shapes))
            self.shapes = self.redo_stack.pop()
            self._clear_selection()
            self.shapes_changed.emit()
            self.update()

    def clear_canvas(self):
        self._push_undo()
        self.shapes.clear()
        self._clear_selection()
        self.shapes_changed.emit()
        self.update()

    def _clear_selection(self):
        self._sel_rect = None
        self._sel_pixels = []
        self._move_ghost = []
        self._moving = False

    # ── Bresenham ──
    def _bresenham(self, x0, y0, x1, y1) -> List[Tuple[int,int]]:
        pts = []
        dx = abs(x1 - x0); sx = 1 if x0 < x1 else -1
        dy = -abs(y1 - y0); sy = 1 if y0 < y1 else -1
        err = dx + dy
        while True:
            pts.append((x0, y0))
            if x0 == x1 and y0 == y1: break
            e2 = 2 * err
            if e2 >= dy: err += dy; x0 += sx
            if e2 <= dx: err += dx; y0 += sy
        return pts

    # ── build pixel buffer (used by both render and select) ──
    def _build_pixel_buffer(self, shape_list=None) -> List[List[int]]:
        pixels = [[0] * DISPLAY_W for _ in range(DISPLAY_H)]
        if shape_list is None:
            shape_list = self.shapes
        self._render_shapes_to_buffer(pixels, shape_list)
        return pixels

    def _render_shapes_to_buffer(self, pixels, shapes):
        def set_px(x, y, c=1):
            if 0 <= x < DISPLAY_W and 0 <= y < DISPLAY_H:
                pixels[y][x] = c

        def draw_line_buf(x0, y0, x1, y1, c):
            for px, py in self._bresenham(x0, y0, x1, y1):
                set_px(px, py, c)

        def draw_rect_buf(x, y, w, h, filled, c):
            if filled:
                for dy in range(h):
                    for dx in range(w):
                        set_px(x+dx, y+dy, c)
            else:
                draw_line_buf(x, y, x+w-1, y, c)
                draw_line_buf(x+w-1, y, x+w-1, y+h-1, c)
                draw_line_buf(x+w-1, y+h-1, x, y+h-1, c)
                draw_line_buf(x, y+h-1, x, y, c)

        def draw_circle_buf(cx, cy, r, filled, c):
            if filled:
                for dy in range(-r, r+1):
                    for dx in range(-r, r+1):
                        if dx*dx + dy*dy <= r*r:
                            set_px(cx+dx, cy+dy, c)
            else:
                x, y, err = r, 0, 0
                while x >= y:
                    for pts in [(cx+x, cy+y),(cx-x, cy+y),(cx+x, cy-y),(cx-x, cy-y),
                                (cx+y, cy+x),(cx-y, cy+x),(cx+y, cy-x),(cx-y, cy-x)]:
                        set_px(pts[0], pts[1], c)
                    y += 1; err += 2*y + 1
                    if 2*(err - x) + 1 > 0:
                        x -= 1; err += 2*(1-x)

        for s in shapes:
            k = s.get("kind"); c = s.get("color", 1)
            if k == "pixel": set_px(s["x"], s["y"], c)
            elif k == "line": draw_line_buf(s["x0"], s["y0"], s["x1"], s["y1"], c)
            elif k == "rect": draw_rect_buf(s["x"], s["y"], s["w"], s["h"], s.get("filled", False), c)
            elif k == "circle": draw_circle_buf(s["cx"], s["cy"], s["r"], s.get("filled", False), c)
            elif k == "text":
                render_text_to_buffer(pixels, s["x"], s["y"], s["text"], s.get("font",""), c)

    # ── selection helpers ──
    def _pixels_in_rect(self, rx, ry, rw, rh) -> List[Tuple[int,int]]:
        """Return list of lit pixel coords within the oled rect."""
        buf = self._build_pixel_buffer()
        result = []
        for y in range(max(0,ry), min(DISPLAY_H, ry+rh)):
            for x in range(max(0,rx), min(DISPLAY_W, rx+rw)):
                if buf[y][x]:
                    result.append((x, y))
        return result

    def _shape_pixels(self, s) -> set:
        """Return the set of (x,y) pixel coords that shape s renders to."""
        tmp = [[0]*DISPLAY_W for _ in range(DISPLAY_H)]
        self._render_shapes_to_buffer(tmp, [s])
        return {(x, y) for y in range(DISPLAY_H) for x in range(DISPLAY_W) if tmp[y][x]}

    def _translate_shape(self, s: dict, dx: int, dy: int) -> dict:
        """Return a copy of shape s with its coordinates shifted by (dx, dy).
        Preserves the original shape kind so the code generator keeps emitting
        drawStr / drawFrame / drawCircle etc. instead of individual drawPixel calls.
        """
        import copy
        t = copy.deepcopy(s)
        k = t.get("kind")
        if k == "pixel":
            t["x"] = max(0, min(DISPLAY_W-1, t["x"] + dx))
            t["y"] = max(0, min(DISPLAY_H-1, t["y"] + dy))
        elif k == "line":
            t["x0"] += dx; t["y0"] += dy
            t["x1"] += dx; t["y1"] += dy
        elif k == "rect":
            t["x"] += dx; t["y"] += dy
        elif k == "circle":
            t["cx"] += dx; t["cy"] += dy
        elif k == "text":
            t["x"] += dx; t["y"] += dy
        return t

    def _commit_move(self, dx, dy):
        """Move selected pixels/shapes by (dx, dy), preserving shape types.

        For each shape in the list we check whether ALL of its rendered pixels
        fall inside the selection. If so, translate its coordinates directly
        (rect stays a rect, text stays text, etc.).

        If only SOME pixels of a shape are selected (partial overlap), we
        rasterise it, keep the non-selected pixels as individual pixel shapes,
        and move the selected pixels.

        Plain pixel shapes are always moved directly.
        """
        if not self._sel_pixels:
            return
        import copy
        sel_set = set(self._sel_pixels)

        self._push_undo()

        new_shapes = []
        placed = set()   # new pixel positions already added (avoid duplicates)

        for s in self.shapes:
            k = s.get("kind")

            if k == "pixel":
                pos = (s["x"], s["y"])
                if pos in sel_set:
                    # This pixel is selected — translate and add later (dedup)
                    nx = max(0, min(DISPLAY_W-1, s["x"] + dx))
                    ny = max(0, min(DISPLAY_H-1, s["y"] + dy))
                    if (nx, ny) not in placed:
                        new_shapes.append({"kind":"pixel","x":nx,"y":ny,"color":s.get("color",1)})
                        placed.add((nx, ny))
                else:
                    new_shapes.append(s)
                continue

            # High-level shape: check overlap with selection
            shape_px = self._shape_pixels(s)
            overlap = shape_px & sel_set

            if not overlap:
                # Entirely outside selection — keep as-is
                new_shapes.append(s)
            elif shape_px <= sel_set:
                # Entirely inside selection — translate the whole shape, preserving type
                new_shapes.append(self._translate_shape(s, dx, dy))
            else:
                # Partial overlap — split: non-selected pixels stay as pixels,
                # selected pixels move. The shape type is lost for the moved part,
                # but this is an unavoidable consequence of partial selection.
                c = s.get("color", 1)
                # Keep non-selected pixels from this shape
                for (px, py) in shape_px - sel_set:
                    new_shapes.append({"kind":"pixel","x":px,"y":py,"color":c})
                # Move selected pixels
                for (px, py) in overlap:
                    nx = max(0, min(DISPLAY_W-1, px + dx))
                    ny = max(0, min(DISPLAY_H-1, py + dy))
                    if (nx, ny) not in placed:
                        new_shapes.append({"kind":"pixel","x":nx,"y":ny,"color":c})
                        placed.add((nx, ny))

        self.shapes = new_shapes

        # Update selection to new pixel positions
        new_sel = []
        seen = set()
        for (x, y) in self._sel_pixels:
            nx = max(0, min(DISPLAY_W-1, x + dx))
            ny = max(0, min(DISPLAY_H-1, y + dy))
            if (nx, ny) not in seen:
                new_sel.append((nx, ny))
                seen.add((nx, ny))
        self._sel_pixels = new_sel

        if self._sel_rect:
            self._sel_rect = QRect(
                max(0, self._sel_rect.x() + dx),
                max(0, self._sel_rect.y() + dy),
                self._sel_rect.width(),
                self._sel_rect.height()
            )

        self.shapes_changed.emit()
        self.update()

    # ── mouse events ──
    def mousePressEvent(self, ev):
        if ev.button() != Qt.MouseButton.LeftButton: return
        self._drawing = True
        self._start = ev.pos()
        self._current = ev.pos()
        ox, oy = self.px_to_oled(ev.pos())

        if self.tool == Tool.TEXT:
            self._add_text(ox, oy)
            self._drawing = False
            return

        if self.tool == Tool.SELECT:
            # Click inside existing selection → start move drag
            if self._sel_rect and self._sel_pixels and self._sel_rect.contains(ox, oy):
                self._moving = True
                self._move_origin = (ox, oy)
                self._move_ghost = list(self._sel_pixels)
                self.setCursor(Qt.CursorShape.SizeAllCursor)
                return
            # Click outside → clear selection and begin a fresh marquee
            self._clear_selection()
            self.update()
            # Fall through: _drawing is already True, marquee will be drawn
            return

        if self.tool == Tool.MOVE:
            # In move mode, always start a move drag from any pixel
            self._moving = True
            self._move_origin = (ox, oy)
            # If no selection, auto-select all pixels
            if not self._sel_pixels:
                self._sel_pixels = self._pixels_in_rect(0, 0, DISPLAY_W, DISPLAY_H)
                self._sel_rect = QRect(0, 0, DISPLAY_W, DISPLAY_H)
            self._move_ghost = list(self._sel_pixels)
            self.setCursor(Qt.CursorShape.SizeAllCursor)
            return

        if self.tool in (Tool.PEN, Tool.ERASER):
            self._push_undo()
            c = 0 if self.tool == Tool.ERASER else self.draw_color
            self._pen_pixels = [{"kind": "pixel", "x": ox, "y": oy, "color": c}]
            self.shapes.append({"kind": "pixel", "x": ox, "y": oy, "color": c})
            self.shapes_changed.emit()
            self.update()

    def mouseMoveEvent(self, ev):
        self._current = ev.pos()
        ox, oy = self.px_to_oled(ev.pos())
        self.coord_changed.emit(ox, oy)

        if not self._drawing:
            self.update()
            return

        # Move tool drag
        if self._moving and self._move_origin:
            dx = ox - self._move_origin[0]
            dy = oy - self._move_origin[1]
            self._move_ghost = [
                (max(0, min(DISPLAY_W-1, x+dx)), max(0, min(DISPLAY_H-1, y+dy)))
                for (x, y) in self._sel_pixels
            ]
            self.update()
            return

        if self.tool in (Tool.PEN, Tool.ERASER):
            c = 0 if self.tool == Tool.ERASER else self.draw_color
            if self._pen_pixels:
                last = self._pen_pixels[-1]
                pts = self._bresenham(last["x"], last["y"], ox, oy)
                for px, py in pts[1:]:
                    if 0 <= px < DISPLAY_W and 0 <= py < DISPLAY_H:
                        self.shapes.append({"kind": "pixel", "x": px, "y": py, "color": c})
                        self._pen_pixels.append({"kind": "pixel", "x": px, "y": py, "color": c})
            self.shapes_changed.emit()
        self.update()

    def mouseReleaseEvent(self, ev):
        if ev.button() != Qt.MouseButton.LeftButton: return
        if not self._drawing: return
        self._drawing = False

        ox0, oy0 = self.px_to_oled(self._start)
        ox1, oy1 = self.px_to_oled(self._current)

        # Finish move
        if self._moving:
            self._moving = False
            self.setCursor(Qt.CursorShape.CrossCursor)
            if self._move_origin:
                dx = ox1 - self._move_origin[0]
                dy = oy1 - self._move_origin[1]
                if dx != 0 or dy != 0:
                    self._commit_move(dx, dy)
                else:
                    # Zero-delta click inside selection = deselect
                    self._clear_selection()
            self._move_ghost = []
            self._move_origin = None
            self.update()
            return

        if self.tool == Tool.SELECT:
            rx = min(ox0, ox1); ry = min(oy0, oy1)
            rw = abs(ox1 - ox0) + 1; rh = abs(oy1 - oy0) + 1
            # A single-pixel "click" with no drag = deselect
            if rw <= 1 and rh <= 1:
                self._clear_selection()
                self.update()
                return
            # Real marquee drag → finalise selection
            self._sel_rect = QRect(rx, ry, rw, rh)
            self._sel_pixels = self._pixels_in_rect(rx, ry, rw, rh)
            self.update()
            return

        if self.tool == Tool.LINE:
            self._push_undo()
            self.shapes.append({"kind": "line", "x0": ox0, "y0": oy0, "x1": ox1, "y1": oy1, "color": self.draw_color})
            self.shapes_changed.emit()

        elif self.tool in (Tool.RECT, Tool.RECT_FILL):
            x = min(ox0, ox1); y = min(oy0, oy1)
            w = abs(ox1 - ox0); h = abs(oy1 - oy0)
            if w > 0 and h > 0:
                self._push_undo()
                self.shapes.append({"kind": "rect", "x": x, "y": y, "w": w, "h": h,
                                    "filled": self.tool == Tool.RECT_FILL,
                                    "color": self.draw_color})
                self.shapes_changed.emit()

        elif self.tool in (Tool.CIRCLE, Tool.CIRCLE_FILL):
            r = int(math.hypot(ox1 - ox0, oy1 - oy0))
            if r > 0:
                self._push_undo()
                self.shapes.append({"kind": "circle", "cx": ox0, "cy": oy0, "r": r,
                                    "filled": self.tool == Tool.CIRCLE_FILL,
                                    "color": self.draw_color})
                self.shapes_changed.emit()

        self._pen_pixels = []
        self.update()

    def keyPressEvent(self, ev):
        """Arrow keys nudge selection by 1 pixel."""
        if self._sel_pixels and self.tool in (Tool.SELECT, Tool.MOVE):
            d = {Qt.Key.Key_Left: (-1,0), Qt.Key.Key_Right: (1,0),
                 Qt.Key.Key_Up: (0,-1), Qt.Key.Key_Down: (0,1)}
            delta = d.get(ev.key())
            if delta:
                self._commit_move(*delta)
                return
        super().keyPressEvent(ev)

    def _add_text(self, ox, oy):
        dlg = TextInputDialog(self.selected_font, self)
        if dlg.exec() == QDialog.DialogCode.Accepted:
            txt = dlg.text_edit.text()
            font = dlg.font_combo.currentText()
            if txt:
                self._push_undo()
                self.shapes.append({"kind": "text", "x": ox, "y": oy,
                                    "text": txt, "font": font, "color": self.draw_color})
                self.shapes_changed.emit()
                self.update()

    # ── rendering ──
    def paintEvent(self, ev):
        p = QPainter(self)
        p.setRenderHint(QPainter.RenderHint.Antialiasing, False)

        # Background
        p.fillRect(0, 0, CANVAS_W, CANVAS_H, QColor(10, 10, 10))

        # Build pixel buffer
        pixels = self._build_pixel_buffer()

        # If moving, blank out original positions and draw ghost
        ghost_set = set()
        orig_set = set(self._sel_pixels)
        if self._move_ghost:
            ghost_set = set(self._move_ghost)

        on_color  = QColor(200, 230, 200)
        ghost_color = QColor(100, 180, 255, 200)
        dim_color = QColor(60, 80, 60)

        for y in range(DISPLAY_H):
            for x in range(DISPLAY_W):
                val = pixels[y][x]
                pos = (x, y)
                if val:
                    if self._move_ghost and pos in orig_set and pos not in ghost_set:
                        # Original position dimmed during drag
                        c = dim_color
                    else:
                        c = on_color
                    px = x * PIXEL_SIZE
                    py = y * PIXEL_SIZE
                    p.fillRect(px, py, PIXEL_SIZE - 1, PIXEL_SIZE - 1, c)

        # Draw ghost pixels during move
        for (x, y) in ghost_set:
            if not (0 <= x < DISPLAY_W and 0 <= y < DISPLAY_H): continue
            p.fillRect(x*PIXEL_SIZE, y*PIXEL_SIZE, PIXEL_SIZE-1, PIXEL_SIZE-1, ghost_color)

        # Highlight selected pixels with a tint
        if self._sel_pixels and not self._move_ghost:
            highlight = QColor(80, 130, 255, 80)
            for (x, y) in self._sel_pixels:
                p.fillRect(x*PIXEL_SIZE, y*PIXEL_SIZE, PIXEL_SIZE-1, PIXEL_SIZE-1, highlight)

        # Draw preview shape while dragging
        if self._drawing and self._start and self._current:
            self._draw_preview(p)

        # Selection rectangle marquee
        if self.tool == Tool.SELECT and self._drawing and self._start and self._current:
            pass  # already drawn in _draw_preview

        # Draw committed selection rect border
        if self._sel_rect and not self._drawing:
            pen = QPen(QColor(80, 160, 255), 1, Qt.PenStyle.DashLine)
            p.setPen(pen)
            p.setBrush(Qt.BrushStyle.NoBrush)
            p.drawRect(
                self._sel_rect.x() * PIXEL_SIZE,
                self._sel_rect.y() * PIXEL_SIZE,
                self._sel_rect.width() * PIXEL_SIZE,
                self._sel_rect.height() * PIXEL_SIZE
            )

        # Grid
        if self.show_grid:
            p.setPen(QPen(QColor(35, 35, 35), 1))
            for x in range(0, CANVAS_W, PIXEL_SIZE):
                p.drawLine(x, 0, x, CANVAS_H)
            for y in range(0, CANVAS_H, PIXEL_SIZE):
                p.drawLine(0, y, CANVAS_W, y)

        p.end()

    def _draw_preview(self, p: QPainter):
        color = QColor(150, 220, 150, 180)
        pen = QPen(color, 1, Qt.PenStyle.DashLine)
        p.setPen(pen)
        p.setBrush(Qt.BrushStyle.NoBrush)

        ox0, oy0 = self.px_to_oled(self._start)
        ox1, oy1 = self.px_to_oled(self._current)

        if self.tool == Tool.SELECT:
            x = min(self._start.x(), self._current.x())
            y = min(self._start.y(), self._current.y())
            w = abs(self._current.x() - self._start.x())
            h = abs(self._current.y() - self._start.y())
            p.setPen(QPen(QColor(80, 160, 255, 200), 1, Qt.PenStyle.DashLine))
            p.drawRect(x, y, w, h)
        elif self.tool == Tool.LINE:
            p.drawLine(
                ox0 * PIXEL_SIZE + PIXEL_SIZE // 2,
                oy0 * PIXEL_SIZE + PIXEL_SIZE // 2,
                ox1 * PIXEL_SIZE + PIXEL_SIZE // 2,
                oy1 * PIXEL_SIZE + PIXEL_SIZE // 2,
            )
        elif self.tool in (Tool.RECT, Tool.RECT_FILL):
            x = min(self._start.x(), self._current.x())
            y = min(self._start.y(), self._current.y())
            w = abs(self._current.x() - self._start.x())
            h = abs(self._current.y() - self._start.y())
            p.drawRect(x, y, w, h)
        elif self.tool in (Tool.CIRCLE, Tool.CIRCLE_FILL):
            r = int(math.hypot(self._current.x() - self._start.x(),
                               self._current.y() - self._start.y()))
            p.drawEllipse(
                self._start.x() - r, self._start.y() - r, r*2, r*2
            )

    def load_shapes(self, shapes: List[dict]):
        import copy
        self.shapes = copy.deepcopy(shapes)
        self.undo_stack.clear()
        self.redo_stack.clear()
        self._clear_selection()
        self.update()


# ─── TEXT INPUT DIALOG ────────────────────────────────────────────────────────
class TextInputDialog(QDialog):
    def __init__(self, default_font, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Insert Text")
        self.setFixedWidth(380)
        self.setStyleSheet("""
            QDialog { background: #1a1a2e; color: #e0e0e0; }
            QLabel { color: #9090b0; font-size: 11px; }
            QLineEdit { background: #0d0d1a; border: 1px solid #3a3a5c;
                        border-radius: 4px; color: #e0e0ff; padding: 6px;
                        font-size: 13px; }
            QComboBox { background: #0d0d1a; border: 1px solid #3a3a5c;
                        border-radius: 4px; color: #e0e0ff; padding: 4px; }
            QPushButton { background: #2d2d5c; border: none; border-radius: 5px;
                          color: #c0d0ff; padding: 8px 18px; font-size: 12px; }
            QPushButton:hover { background: #3a3a7a; }
        """)

        lay = QVBoxLayout(self)
        lay.setSpacing(12)
        lay.setContentsMargins(20, 20, 20, 20)

        lay.addWidget(QLabel("Text content:"))
        self.text_edit = QLineEdit()
        self.text_edit.setPlaceholderText("Enter text...")
        lay.addWidget(self.text_edit)

        lay.addWidget(QLabel("Font:"))
        self.font_combo = QComboBox()
        self.font_combo.addItems(U8G2_FONTS)
        idx = self.font_combo.findText(default_font)
        if idx >= 0: self.font_combo.setCurrentIndex(idx)
        lay.addWidget(self.font_combo)

        btns = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        btns.accepted.connect(self.accept)
        btns.rejected.connect(self.reject)
        lay.addWidget(btns)
        self.text_edit.setFocus()


# ─── TOOL BUTTON ──────────────────────────────────────────────────────────────
class ToolBtn(QPushButton):
    def __init__(self, label, tooltip, parent=None):
        super().__init__(label, parent)
        self.setToolTip(tooltip)
        self.setCheckable(True)
        self.setFixedSize(56, 44)
        self.setStyleSheet("""
            QPushButton {
                background: #12122a;
                border: 1px solid #2a2a4a;
                border-radius: 7px;
                color: #8888bb;
                font-size: 18px;
                font-family: 'Segoe UI Emoji', 'Apple Color Emoji', sans-serif;
            }
            QPushButton:hover { background: #1e1e3a; border-color: #4a4a7a; color: #aaaadd; }
            QPushButton:checked { background: #1a2a5a; border-color: #5577ff; color: #99aaff; }
        """)


# ─── SCREENS PANEL ────────────────────────────────────────────────────────────
class ScreensPanel(QWidget):
    screen_selected = pyqtSignal(int)
    screen_renamed = pyqtSignal(int, str)
    screen_deleted = pyqtSignal(int)
    screen_added = pyqtSignal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFixedWidth(200)

        lay = QVBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(0)

        # Header
        hdr = QLabel("SCREENS")
        hdr.setStyleSheet("""
            background: #0d0d20;
            color: #5566aa;
            font-size: 10px;
            font-weight: bold;
            letter-spacing: 3px;
            padding: 12px 14px;
            border-bottom: 1px solid #1a1a3a;
        """)
        lay.addWidget(hdr)

        # List
        self.list_widget = QListWidget()
        self.list_widget.setStyleSheet("""
            QListWidget {
                background: #0d0d20;
                border: none;
                color: #b0b0d0;
                font-size: 12px;
                outline: none;
            }
            QListWidget::item {
                padding: 9px 14px;
                border-bottom: 1px solid #141430;
            }
            QListWidget::item:hover { background: #161630; }
            QListWidget::item:selected {
                background: #1a1a4a;
                color: #8899ff;
                border-left: 3px solid #5566ff;
            }
        """)
        self.list_widget.currentRowChanged.connect(self.screen_selected.emit)
        self.list_widget.itemDoubleClicked.connect(self._rename)
        lay.addWidget(self.list_widget)

        # Buttons
        btn_row = QWidget()
        btn_row.setStyleSheet("background: #0d0d20; border-top: 1px solid #1a1a3a;")
        blay = QHBoxLayout(btn_row)
        blay.setContentsMargins(8, 8, 8, 8)
        blay.setSpacing(6)

        for icon, tip, fn in [
            ("＋", "Add screen", self._add),
            ("⎘", "Duplicate", self._dup),
            ("✕", "Delete", self._del),
        ]:
            b = QPushButton(icon)
            b.setToolTip(tip)
            b.setFixedSize(38, 30)
            b.setStyleSheet("""
                QPushButton { background: #161630; border: 1px solid #2a2a4a;
                              border-radius: 5px; color: #7788bb; font-size: 14px; }
                QPushButton:hover { background: #202048; color: #aabbff; }
            """)
            b.clicked.connect(fn)
            blay.addWidget(b)

        lay.addWidget(btn_row)

    def add_screen(self, name: str):
        item = QListWidgetItem(name)
        self.list_widget.addItem(item)
        self.list_widget.setCurrentRow(self.list_widget.count() - 1)

    def set_screen_name(self, idx: int, name: str):
        if 0 <= idx < self.list_widget.count():
            self.list_widget.item(idx).setText(name)

    def _add(self):
        self.screen_added.emit()

    def _dup(self):
        row = self.list_widget.currentRow()
        if row >= 0:
            self.screen_added.emit()

    def _del(self):
        row = self.list_widget.currentRow()
        if row >= 0:
            self.screen_deleted.emit(row)

    def _rename(self, item):
        row = self.list_widget.row(item)
        name, ok = QInputDialog.getText(self, "Rename Screen", "Screen name:", text=item.text())
        if ok and name:
            self.screen_renamed.emit(row, name)


# ─── MAIN WINDOW ──────────────────────────────────────────────────────────────
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("U8g2 Screen Designer  —  SH1106 128×64")
        self.setMinimumSize(1100, 780)

        self.screens: List[Screen] = []
        self.current_idx = -1
        self._loading = False

        self._build_ui()
        self._setup_shortcuts()

        # Start with one blank screen
        self._add_screen()

    # ── UI BUILD ──────────────────────────────────────────────────────────────
    def _build_ui(self):
        self.setStyleSheet("""
            QMainWindow, QWidget { background: #10101e; color: #d0d0e8; }
            QSplitter::handle { background: #1a1a32; }
            QScrollArea { border: none; background: transparent; }
            QTextEdit {
                background: #0a0a18;
                border: 1px solid #2a2a4a;
                border-radius: 6px;
                color: #7dda8a;
                font-family: 'JetBrains Mono', 'Cascadia Code', 'Fira Code', monospace;
                font-size: 12px;
                padding: 10px;
                selection-background-color: #2a3a6a;
            }
            QLabel { color: #8888aa; }
            QComboBox {
                background: #12122a; border: 1px solid #2a2a4a;
                border-radius: 5px; color: #c0c0e0; padding: 4px 8px;
            }
            QComboBox::drop-down { border: none; }
            QComboBox QAbstractItemView {
                background: #12122a; border: 1px solid #3a3a6a;
                color: #c0c0e0; selection-background-color: #2a2a5a;
            }
            QGroupBox {
                border: 1px solid #1e1e3a; border-radius: 7px;
                margin-top: 8px; color: #5566aa;
                font-size: 9px; font-weight: bold; letter-spacing: 2px;
            }
            QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
            QSpinBox {
                background: #12122a; border: 1px solid #2a2a4a;
                border-radius: 4px; color: #c0c0e0; padding: 3px;
            }
            QCheckBox { color: #8888bb; spacing: 6px; }
            QCheckBox::indicator { width: 14px; height: 14px;
                border: 1px solid #3a3a6a; border-radius: 3px; background: #12122a; }
            QCheckBox::indicator:checked { background: #3355cc; border-color: #5577ff; }
            QSlider::groove:horizontal { height: 4px; background: #1e1e3a; border-radius: 2px; }
            QSlider::handle:horizontal { width: 14px; height: 14px; background: #5566cc;
                border-radius: 7px; margin: -5px 0; }
            QScrollBar:vertical { background: #0d0d1a; width: 8px; }
            QScrollBar::handle:vertical { background: #2a2a4a; border-radius: 4px; min-height: 20px; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        """)

        central = QWidget()
        self.setCentralWidget(central)
        root = QHBoxLayout(central)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        # ── Left: screens panel ──
        self.screens_panel = ScreensPanel()
        self.screens_panel.screen_selected.connect(self._on_screen_selected)
        self.screens_panel.screen_renamed.connect(self._on_screen_renamed)
        self.screens_panel.screen_deleted.connect(self._on_screen_deleted)
        self.screens_panel.screen_added.connect(self._add_screen)
        root.addWidget(self.screens_panel)

        # Separator
        sep = QFrame(); sep.setFrameShape(QFrame.Shape.VLine)
        sep.setStyleSheet("color: #1a1a3a; background: #1a1a3a; max-width: 1px;")
        root.addWidget(sep)

        # ── Center + right ──
        main_splitter = QSplitter(Qt.Orientation.Horizontal)
        main_splitter.setHandleWidth(2)
        root.addWidget(main_splitter)

        # Center: toolbar + canvas + code
        center = QWidget()
        clayout = QVBoxLayout(center)
        clayout.setContentsMargins(16, 12, 16, 12)
        clayout.setSpacing(10)

        # Title bar area
        title_row = QHBoxLayout()
        title = QLabel("U8g2 Screen Designer")
        title.setStyleSheet("font-size: 15px; font-weight: bold; color: #6677cc; letter-spacing: 1px;")
        title_row.addWidget(title)
        title_row.addStretch()

        # Export buttons
        for lbl, tip, fn in [
            ("Export JSON", "Save all screens to JSON file", self._export_json),
            ("Import JSON", "Load screens from JSON file", self._import_json),
        ]:
            b = QPushButton(lbl)
            b.setToolTip(tip)
            b.setFixedHeight(30)
            b.setStyleSheet("""
                QPushButton { background: #12122a; border: 1px solid #2a2a4a;
                              border-radius: 5px; color: #8899bb; font-size: 11px; padding: 0 12px; }
                QPushButton:hover { background: #1e1e3a; color: #aabbdd; }
            """)
            b.clicked.connect(fn)
            title_row.addWidget(b)

        clayout.addLayout(title_row)

        # Create canvas FIRST so _build_toolbar() can reference self.canvas
        self.canvas = OLEDCanvas()
        self.canvas.shapes_changed.connect(self._on_shapes_changed)
        self.canvas.coord_changed.connect(self._on_coord_changed)
        self._code_manually_edited = False
        self._updating_code = False

        # Tool bar
        clayout.addWidget(self._build_toolbar())

        # Canvas + coordinates label
        canvas_frame = QFrame()
        canvas_frame.setStyleSheet("""
            QFrame { background: #0a0a18; border: 1px solid #2a2a4a; border-radius: 8px; }
        """)
        cf_lay = QVBoxLayout(canvas_frame)
        cf_lay.setContentsMargins(10, 10, 10, 10)
        cf_lay.setSpacing(8)

        # Canvas scroll
        scroll = QScrollArea()
        scroll.setWidgetResizable(False)
        scroll.setStyleSheet("QScrollArea { border: none; background: transparent; }")

        scroll.setWidget(self.canvas)
        scroll.setFixedSize(CANVAS_W + 20, CANVAS_H + 20)
        cf_lay.addWidget(scroll, alignment=Qt.AlignmentFlag.AlignCenter)

        # Coordinates + size note
        info_row = QHBoxLayout()
        self.coord_label = QLabel("Hover to see coordinates")
        self.coord_label.setStyleSheet("font-size: 10px; color: #445566;")
        info_row.addWidget(self.coord_label)
        info_row.addStretch()
        info_row.addWidget(QLabel("128 × 64 px  |  SH1106  |  U8g2"))
        cf_lay.addLayout(info_row)

        clayout.addWidget(canvas_frame)

        # Code output
        code_label = QLabel("GENERATED CODE")
        code_label.setStyleSheet("font-size: 9px; font-weight: bold; letter-spacing: 3px; color: #445566;")
        clayout.addWidget(code_label)

        code_note = QLabel("Editable — changes here are for export only and won't update the canvas")
        code_note.setStyleSheet("font-size: 9px; color: #334455; font-style: italic;")
        clayout.addWidget(code_note)

        self.code_edit = QTextEdit()
        self.code_edit.setReadOnly(False)
        self.code_edit.setMinimumHeight(160)
        self.code_edit.setStyleSheet("""
            QTextEdit {
                background: #0a0a18;
                border: 1px solid #2a2a4a;
                border-radius: 6px;
                color: #7dda8a;
                font-family: 'JetBrains Mono', 'Cascadia Code', 'Fira Code', monospace;
                font-size: 12px;
                padding: 10px;
                selection-background-color: #2a3a6a;
            }
        """)
        self.code_edit.textChanged.connect(self._on_code_text_changed)
        clayout.addWidget(self.code_edit)

        code_btn_row = QHBoxLayout()
        copy_btn = QPushButton("Copy Code")
        copy_btn.setFixedHeight(30)
        copy_btn.setStyleSheet("""
            QPushButton { background: #1a2a4a; border: 1px solid #3355aa;
                          border-radius: 5px; color: #8899cc; font-size: 11px; padding: 0 12px; }
            QPushButton:hover { background: #223366; color: #bbccff; }
        """)
        copy_btn.clicked.connect(lambda: QApplication.clipboard().setText(self.code_edit.toPlainText()))

        reset_btn = QPushButton("↺ Reset to Canvas")
        reset_btn.setFixedHeight(30)
        reset_btn.setToolTip("Discard manual edits and regenerate from canvas")
        reset_btn.setStyleSheet("""
            QPushButton { background: #1a1a2a; border: 1px solid #2a2a4a;
                          border-radius: 5px; color: #667788; font-size: 11px; padding: 0 12px; }
            QPushButton:hover { background: #1e2030; color: #9aabcc; }
        """)
        reset_btn.clicked.connect(self._force_refresh_code)

        code_btn_row.addStretch()
        code_btn_row.addWidget(reset_btn)
        code_btn_row.addWidget(copy_btn)
        clayout.addLayout(code_btn_row)

        main_splitter.addWidget(center)

        # ── Right: properties panel ──
        main_splitter.addWidget(self._build_props_panel())
        main_splitter.setSizes([820, 220])

    def _build_toolbar(self) -> QWidget:
        bar = QWidget()
        bar.setStyleSheet("background: #0d0d20; border-radius: 8px; padding: 4px;")
        lay = QHBoxLayout(bar)
        lay.setContentsMargins(8, 6, 8, 6)
        lay.setSpacing(4)

        self._tool_group = QButtonGroup(self)
        tools = [
            ("⬚", Tool.SELECT,       "Select pixels (drag marquee, then drag to move)  [S]"),
            ("⤢", Tool.MOVE,         "Move all / selection  [M]"),
            ("✏️", Tool.PEN,         "Pen / freehand draw  [P]"),
            ("╱", Tool.LINE,         "Line  [L]"),
            ("▭", Tool.RECT,         "Rectangle (outline)  [R]"),
            ("▬", Tool.RECT_FILL,    "Rectangle (filled)  [F]"),
            ("○", Tool.CIRCLE,       "Circle (outline)  [C]"),
            ("●", Tool.CIRCLE_FILL,  "Circle (filled)  [O]"),
            ("T", Tool.TEXT,         "Text  [T]"),
            ("⌫", Tool.ERASER,       "Eraser  [E]"),
        ]

        for icon, tool, tip in tools:
            btn = ToolBtn(icon, tip)
            btn.setProperty("tool", tool)
            self._tool_group.addButton(btn)
            lay.addWidget(btn)
            if tool == Tool.PEN:
                btn.setChecked(True)

        self._tool_group.buttonClicked.connect(self._on_tool_clicked)

        lay.addSpacing(12)

        # Color toggle
        color_lbl = QLabel("Color:")
        color_lbl.setStyleSheet("color: #556688; font-size: 10px;")
        lay.addWidget(color_lbl)

        self.color_on_btn = QPushButton("ON")
        self.color_on_btn.setCheckable(True)
        self.color_on_btn.setChecked(True)
        self.color_on_btn.setFixedSize(44, 32)
        self.color_on_btn.setStyleSheet("""
            QPushButton { background: #1a3a1a; border: 1px solid #2a5a2a;
                          border-radius: 5px; color: #66cc66; font-size: 11px; font-weight: bold; }
            QPushButton:checked { background: #2a5a2a; border-color: #44cc44; }
        """)
        self.color_off_btn = QPushButton("OFF")
        self.color_off_btn.setCheckable(True)
        self.color_off_btn.setFixedSize(44, 32)
        self.color_off_btn.setStyleSheet("""
            QPushButton { background: #1a1a1a; border: 1px solid #3a3a3a;
                          border-radius: 5px; color: #666666; font-size: 11px; font-weight: bold; }
            QPushButton:checked { background: #333333; border-color: #666666; }
        """)

        cg = QButtonGroup(self)
        cg.addButton(self.color_on_btn)
        cg.addButton(self.color_off_btn)
        self.color_on_btn.toggled.connect(lambda c: setattr(self.canvas, 'draw_color', 1 if c else 0))
        lay.addWidget(self.color_on_btn)
        lay.addWidget(self.color_off_btn)

        lay.addSpacing(12)

        # Grid toggle
        self.grid_btn = QPushButton("⊞ Grid")
        self.grid_btn.setCheckable(True)
        self.grid_btn.setChecked(True)
        self.grid_btn.setFixedHeight(32)
        self.grid_btn.setStyleSheet("""
            QPushButton { background: #12122a; border: 1px solid #2a2a4a;
                          border-radius: 5px; color: #5566aa; font-size: 11px; padding: 0 10px; }
            QPushButton:checked { background: #1a1a40; border-color: #4455cc; color: #8899ff; }
        """)
        self.grid_btn.toggled.connect(lambda c: (setattr(self.canvas, 'show_grid', c), self.canvas.update()))
        lay.addWidget(self.grid_btn)

        lay.addStretch()

        # Undo/redo
        for icon, tip, fn in [("↩", "Undo (Ctrl+Z)", self.canvas.undo),
                               ("↪", "Redo (Ctrl+Y)", self.canvas.redo),
                               ("🗑 Clear", "Clear canvas", self._clear_confirm)]:
            b = QPushButton(icon)
            b.setToolTip(tip)
            b.setFixedHeight(32)
            b.setStyleSheet("""
                QPushButton { background: #12122a; border: 1px solid #2a2a4a;
                              border-radius: 5px; color: #7788aa; font-size: 13px; padding: 0 10px; }
                QPushButton:hover { background: #1e1e3a; color: #aabbdd; }
            """)
            b.clicked.connect(fn)
            lay.addWidget(b)

        return bar

    def _build_props_panel(self) -> QWidget:
        panel = QWidget()
        panel.setFixedWidth(220)
        panel.setStyleSheet("background: #0d0d1e; border-left: 1px solid #1a1a32;")
        lay = QVBoxLayout(panel)
        lay.setContentsMargins(14, 14, 14, 14)
        lay.setSpacing(14)

        hdr = QLabel("PROPERTIES")
        hdr.setStyleSheet("font-size: 9px; font-weight: bold; letter-spacing: 3px; color: #445566;")
        lay.addWidget(hdr)

        # Font selector
        font_grp = QGroupBox("TEXT FONT")
        fg = QVBoxLayout(font_grp)
        self.font_combo = QComboBox()
        self.font_combo.addItems(U8G2_FONTS)
        idx_5x7 = self.font_combo.findText("u8g2_font_5x7_tr")
        self.font_combo.setCurrentIndex(idx_5x7 if idx_5x7 >= 0 else 1)
        self.font_combo.currentTextChanged.connect(
            lambda t: setattr(self.canvas, 'selected_font', t)
        )
        fg.addWidget(self.font_combo)
        lay.addWidget(font_grp)

        # Shape count
        info_grp = QGroupBox("CANVAS INFO")
        ig = QVBoxLayout(info_grp)
        self.shape_count_lbl = QLabel("Shapes: 0")
        self.shape_count_lbl.setStyleSheet("color: #7788aa; font-size: 11px;")
        ig.addWidget(self.shape_count_lbl)
        lay.addWidget(info_grp)

        # Function name
        fn_grp = QGroupBox("FUNCTION NAME")
        fg2 = QVBoxLayout(fn_grp)
        self.fn_name_edit = QLineEdit("drawScreen")
        self.fn_name_edit.setStyleSheet("""
            QLineEdit { background: #0a0a18; border: 1px solid #2a2a4a;
                        border-radius: 4px; color: #c0d0ff; padding: 5px; font-size: 12px; }
        """)
        self.fn_name_edit.textChanged.connect(self._refresh_code)
        fg2.addWidget(self.fn_name_edit)
        lay.addWidget(fn_grp)

        # Shapes list
        shapes_grp = QGroupBox("SHAPES")
        sg = QVBoxLayout(shapes_grp)
        self.shapes_list = QListWidget()
        self.shapes_list.setFixedHeight(180)
        self.shapes_list.setStyleSheet("""
            QListWidget { background: #0a0a18; border: 1px solid #1e1e38;
                          border-radius: 4px; color: #8888aa; font-size: 10px; }
            QListWidget::item { padding: 3px 6px; }
            QListWidget::item:selected { background: #1a1a40; color: #9999cc; }
        """)
        sg.addWidget(self.shapes_list)

        del_btn = QPushButton("Delete Selected Shape")
        del_btn.setFixedHeight(28)
        del_btn.setStyleSheet("""
            QPushButton { background: #2a1010; border: 1px solid #4a2020;
                          border-radius: 4px; color: #aa6666; font-size: 10px; }
            QPushButton:hover { background: #3a1818; color: #cc8888; }
        """)
        del_btn.clicked.connect(self._delete_selected_shape)
        sg.addWidget(del_btn)
        lay.addWidget(shapes_grp)

        lay.addStretch()
        return panel

    # ── SHORTCUTS ─────────────────────────────────────────────────────────────
    def _setup_shortcuts(self):
        QShortcut(QKeySequence("Ctrl+Z"), self, self.canvas.undo)
        QShortcut(QKeySequence("Ctrl+Y"), self, self.canvas.redo)
        QShortcut(QKeySequence("Ctrl+Shift+Z"), self, self.canvas.redo)
        QShortcut(QKeySequence("Ctrl+S"), self, self._export_json)

        # Tool keys
        for key, tool in [("S", Tool.SELECT), ("M", Tool.MOVE),
                           ("P", Tool.PEN), ("L", Tool.LINE), ("R", Tool.RECT),
                           ("F", Tool.RECT_FILL), ("C", Tool.CIRCLE),
                           ("O", Tool.CIRCLE_FILL), ("T", Tool.TEXT), ("E", Tool.ERASER)]:
            QShortcut(QKeySequence(key), self, lambda t=tool: self._set_tool(t))

    def _set_tool(self, tool: Tool):
        for btn in self._tool_group.buttons():
            if btn.property("tool") == tool:
                btn.setChecked(True)
                self.canvas.tool = tool
                break

    # ── EVENT HANDLERS ────────────────────────────────────────────────────────
    def _on_tool_clicked(self, btn):
        self.canvas.tool = btn.property("tool")

    def _on_shapes_changed(self):
        if self._loading: return
        if self.current_idx >= 0:
            self.screens[self.current_idx].shapes = list(self.canvas.shapes)
        self._refresh_code()
        self._refresh_shapes_list()

    def _on_coord_changed(self, x, y):
        sel = len(self.canvas._sel_pixels)
        sel_str = f"  |  {sel} px selected" if sel else ""
        self.coord_label.setText(f"({x}, {y}){sel_str}")

    def _refresh_code(self):
        # Don't overwrite manual edits — user must click Reset to re-sync
        if getattr(self, '_code_manually_edited', False):
            shapes = self.canvas.shapes
            self.shape_count_lbl.setText(f"Shapes: {len(shapes)}")
            return
        self._updating_code = True
        shapes = self.canvas.shapes
        fn = self.fn_name_edit.text().strip() or "drawScreen"
        code = generate_code(shapes, fn)
        self.code_edit.setPlainText(code)
        self.shape_count_lbl.setText(f"Shapes: {len(shapes)}")
        self._updating_code = False

    def _force_refresh_code(self):
        """Discard manual edits and regenerate from canvas."""
        self._code_manually_edited = False
        self._updating_code = True
        shapes = self.canvas.shapes
        fn = self.fn_name_edit.text().strip() or "drawScreen"
        code = generate_code(shapes, fn)
        self.code_edit.setPlainText(code)
        self.shape_count_lbl.setText(f"Shapes: {len(shapes)}")
        self._updating_code = False

    def _on_code_text_changed(self):
        """Mark code as manually edited once user types in the box."""
        if not getattr(self, '_updating_code', False):
            self._code_manually_edited = True

    def _refresh_shapes_list(self):
        self.shapes_list.clear()
        for s in self.canvas.shapes:
            k = s.get("kind", "?")
            if k == "pixel":
                desc = f"Pixel ({s['x']},{s['y']})"
            elif k == "line":
                desc = f"Line ({s['x0']},{s['y0']})→({s['x1']},{s['y1']})"
            elif k == "rect":
                ft = "Filled " if s.get("filled") else ""
                desc = f"{ft}Rect ({s['x']},{s['y']}) {s['w']}×{s['h']}"
            elif k == "circle":
                ft = "Disc" if s.get("filled") else "Circle"
                desc = f"{ft} r={s['r']} @({s['cx']},{s['cy']})"
            elif k == "text":
                desc = f'Text "{s["text"]}" @({s["x"]},{s["y"]})'
            else:
                desc = str(s)
            self.shapes_list.addItem(desc)

    def _delete_selected_shape(self):
        row = self.shapes_list.currentRow()
        if row >= 0 and row < len(self.canvas.shapes):
            self.canvas._push_undo()
            self.canvas.shapes.pop(row)
            self.canvas.update()
            self._on_shapes_changed()

    def _clear_confirm(self):
        if QMessageBox.question(
            self, "Clear Canvas", "Clear all shapes from this screen?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        ) == QMessageBox.StandardButton.Yes:
            self.canvas.clear_canvas()

    # ── SCREEN MANAGEMENT ─────────────────────────────────────────────────────
    def _add_screen(self):
        import copy
        if self.current_idx >= 0 and self.screens:
            # Duplicate if triggered from dup button
            src = self.screens[self.current_idx]
            name = f"{src.name} Copy"
            new = Screen(name=name, shapes=copy.deepcopy(src.shapes))
        else:
            n = len(self.screens) + 1
            new = Screen(name=f"Screen {n}", shapes=[])
        self.screens.append(new)
        self.screens_panel.add_screen(new.name)
        self._switch_to(len(self.screens) - 1)

    def _on_screen_selected(self, idx):
        if idx == self.current_idx or idx < 0: return
        self._save_current()
        self._switch_to(idx)

    def _switch_to(self, idx):
        if idx < 0 or idx >= len(self.screens): return
        self._loading = True
        self.current_idx = idx
        self.canvas.load_shapes(self.screens[idx].shapes)
        self._loading = False
        self._refresh_code()
        self._refresh_shapes_list()
        self.fn_name_edit.setText(
            self.screens[idx].name.replace(" ", "_").lower()
        )
        self.screens_panel.list_widget.setCurrentRow(idx)

    def _save_current(self):
        if self.current_idx >= 0:
            self.screens[self.current_idx].shapes = list(self.canvas.shapes)

    def _on_screen_renamed(self, idx, name):
        if 0 <= idx < len(self.screens):
            self.screens[idx].name = name
            self.screens_panel.set_screen_name(idx, name)

    def _on_screen_deleted(self, idx):
        if len(self.screens) <= 1:
            QMessageBox.information(self, "Can't Delete", "You need at least one screen.")
            return
        self.screens.pop(idx)
        self.screens_panel.list_widget.takeItem(idx)
        new_idx = min(idx, len(self.screens) - 1)
        self.current_idx = -1
        self._switch_to(new_idx)

    # ── IMPORT / EXPORT ───────────────────────────────────────────────────────
    def _export_json(self):
        self._save_current()
        path, _ = QFileDialog.getSaveFileName(
            self, "Save Screens", "screens.json", "JSON Files (*.json)"
        )
        if path:
            data = [s.to_dict() for s in self.screens]
            with open(path, "w") as f:
                json.dump(data, f, indent=2)
            QMessageBox.information(self, "Saved", f"Saved {len(self.screens)} screen(s) to:\n{path}")

    def _import_json(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Load Screens", "", "JSON Files (*.json)"
        )
        if not path: return
        try:
            with open(path) as f:
                data = json.load(f)
            self.screens.clear()
            self.screens_panel.list_widget.clear()
            self.current_idx = -1
            for d in data:
                s = Screen.from_dict(d)
                self.screens.append(s)
                self.screens_panel.add_screen(s.name)
            if self.screens:
                self._switch_to(0)
            QMessageBox.information(self, "Loaded", f"Loaded {len(self.screens)} screen(s).")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to load:\n{e}")

    # ── MOUSE TRACKING FOR COORD DISPLAY ──────────────────────────────────────
    def mouseMoveEvent(self, ev):
        super().mouseMoveEvent(ev)


# ─── ENTRY POINT ──────────────────────────────────────────────────────────────
def main():
    app = QApplication(sys.argv)
    app.setApplicationName("U8g2 Screen Designer")

    # Dark palette
    palette = QPalette()
    palette.setColor(QPalette.ColorRole.Window, QColor(16, 16, 30))
    palette.setColor(QPalette.ColorRole.WindowText, QColor(200, 200, 220))
    palette.setColor(QPalette.ColorRole.Base, QColor(10, 10, 20))
    palette.setColor(QPalette.ColorRole.AlternateBase, QColor(15, 15, 25))
    palette.setColor(QPalette.ColorRole.Text, QColor(200, 200, 220))
    palette.setColor(QPalette.ColorRole.Button, QColor(18, 18, 35))
    palette.setColor(QPalette.ColorRole.ButtonText, QColor(180, 180, 210))
    palette.setColor(QPalette.ColorRole.Highlight, QColor(50, 70, 180))
    palette.setColor(QPalette.ColorRole.HighlightedText, QColor(220, 230, 255))
    app.setPalette(palette)

    win = MainWindow()
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()