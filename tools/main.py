"""
Protogen MAX7219 Bitmap Editor  –  PyQt6
Each array value = one COLUMN byte (bit 0 = top row, bit 7 = bottom row).
"""

import sys
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QHBoxLayout, QVBoxLayout,
    QLabel, QPushButton, QButtonGroup, QFrame, QTextEdit,
    QScrollArea, QInputDialog, QSizePolicy,
)
from PyQt6.QtCore import Qt, QRect, pyqtSignal
from PyQt6.QtGui import (
    QPainter, QColor, QPen, QFont,
    QPalette,
)

# ── Data ──────────────────────────────────────────────────────────────────────

PATTERNS = {
    "eye": {
        "cols": 16, "rows": 8,
        "var": "uint8_t eye[16]",
        "data": [
            0x30, 0x18, 0x18, 0x0C, 0x0C, 0x0E, 0x0E, 0x0E,
            0x0F, 0x0F, 0x0F, 0x0F, 0x1E, 0x3E, 0x3C, 0x18,
        ],
    },
    "mouth": {
        "cols": 32, "rows": 8,
        "var": "uint8_t protogenMouth[32]",
        "data": [
            0x1C, 0x1E, 0x13, 0x16, 0x16, 0x1C, 0x1C, 0x18,
            0x18, 0x30, 0x30, 0x60, 0x60, 0xC0, 0xC0, 0x80,
            0x80, 0xC0, 0xC0, 0x60, 0x60, 0x30, 0x30, 0x10,
            0x18, 0x18, 0x18, 0x30, 0x30, 0x60, 0x60, 0xC0,
        ],
    },
    "nose": {
        "cols": 8, "rows": 8,
        "var": "uint8_t nose[8]",
        "data": [
            0x3C, 0x1E, 0x06, 0x06, 0x06, 0x06, 0x02, 0x00,
        ],
    },
}

# ── Theme ─────────────────────────────────────────────────────────────────────

C_BG      = "#0d0f14"
C_PANEL   = "#13161e"
C_CARD    = "#1a1e2a"
C_BORDER  = "#252b3b"
C_ACCENT  = "#00e5ff"
C_ACCENT2 = "#7c3aed"
C_ON      = "#00e5ff"
C_OFF     = "#1a2035"
C_GRID    = "#252b3b"
C_TEXT1   = "#e8eaf6"
C_TEXT2   = "#6b7db3"
C_RED     = "#ff4d6d"

CELL = 36
GAP  = 1

# ── Helpers ───────────────────────────────────────────────────────────────────

def data_to_grid(data, cols, rows):
    grid = [[0] * cols for _ in range(rows)]
    for c, byte in enumerate(data):
        for r in range(rows):
            grid[r][c] = (byte >> r) & 1
    return grid


def grid_to_data(grid, cols, rows):
    out = []
    for c in range(cols):
        byte = 0
        for r in range(rows):
            if grid[r][c]:
                byte |= (1 << r)
        out.append(byte)
    return out


def fmt_array(data, var_name):
    per_row = 8
    lines = []
    for i in range(0, len(data), per_row):
        chunk = data[i:i + per_row]
        lines.append("  " + ", ".join(f"0x{b:02X}" for b in chunk))
    return f"const {var_name} = {{\n" + ",\n".join(lines) + "\n};"


# ── Preset persistence ────────────────────────────────────────────────────────

import json, os, copy, re

PRESET_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "presets.json")

def load_presets():
    """Return {id: {name, base, cols, rows, var, data}} from disk, or {}."""
    if not os.path.exists(PRESET_FILE):
        return {}
    try:
        raw = json.loads(open(PRESET_FILE).read())
        # Validate entries have required keys
        out = {}
        for k, v in raw.items():
            if all(f in v for f in ("name","base","cols","rows","var","data")):
                out[k] = v
        return out
    except Exception:
        return {}

def save_presets(presets):
    with open(PRESET_FILE, "w") as f:
        json.dump(presets, f, indent=2)

def new_preset_id(presets):
    i = 1
    while f"preset_{i}" in presets:
        i += 1
    return f"preset_{i}"


class PixelCanvas(QWidget):
    changed       = pyqtSignal()
    press_started = pyqtSignal()   # fired once on mousePress, before any pixel change

    def __init__(self, parent=None):
        super().__init__(parent)
        self.cols = 8
        self.rows = 8
        self.grid = [[0] * self.cols for _ in range(self.rows)]
        self._drag_val = None
        self.tool = "paint"
        self.setCursor(Qt.CursorShape.CrossCursor)
        self._refresh_size()

    def _refresh_size(self):
        w = self.cols * (CELL + GAP) + GAP
        h = self.rows * (CELL + GAP) + GAP + 20
        self.setFixedSize(w, h)

    def load(self, cols, rows, grid):
        self.cols = cols
        self.rows = rows
        self.grid = [row[:] for row in grid]
        self._refresh_size()
        self.update()

    def get_grid(self):
        return [row[:] for row in self.grid]

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.RenderHint.Antialiasing, False)
        p.fillRect(self.rect(), QColor(C_CARD))

        for r in range(self.rows):
            for c in range(self.cols):
                x = GAP + c * (CELL + GAP)
                y = GAP + r * (CELL + GAP)
                on = self.grid[r][c]
                p.fillRect(x, y, CELL, CELL,
                           QColor(C_ON) if on else QColor(C_OFF))
                if on:
                    p.setPen(QPen(QColor("#40f8ff"), 1))
                else:
                    p.setPen(QPen(QColor(C_GRID), 1))
                p.drawRect(x, y, CELL - 1, CELL - 1)

        # column index labels
        p.setFont(QFont("Courier New", 7))
        p.setPen(QColor(C_TEXT2))
        label_y = GAP + self.rows * (CELL + GAP) + 3
        for c in range(self.cols):
            x = GAP + c * (CELL + GAP)
            p.drawText(QRect(x, label_y, CELL, 14),
                       Qt.AlignmentFlag.AlignHCenter, str(c))
        p.end()

    def _cell(self, pos):
        c = pos.x() // (CELL + GAP)
        r = pos.y() // (CELL + GAP)
        if 0 <= r < self.rows and 0 <= c < self.cols:
            return r, c
        return None

    def mousePressEvent(self, e):
        if e.button() != Qt.MouseButton.LeftButton:
            return
        pos = self._cell(e.pos())
        if pos is None:
            return
        r, c = pos
        self._drag_val = 0 if (self.tool == "erase" or self.grid[r][c]) else 1
        if self.tool == "erase":
            self._drag_val = 0
        self.press_started.emit()   # snapshot BEFORE the first pixel changes
        self._apply(r, c)

    def mouseMoveEvent(self, e):
        if self._drag_val is None:
            return
        pos = self._cell(e.pos())
        if pos:
            self._apply(*pos)

    def mouseReleaseEvent(self, _):
        self._drag_val = None

    def _apply(self, r, c):
        if self.grid[r][c] == self._drag_val:
            return
        self.grid[r][c] = self._drag_val
        self.update()
        self.changed.emit()


# ── UI helpers ────────────────────────────────────────────────────────────────

def divider():
    f = QFrame()
    f.setFrameShape(QFrame.Shape.HLine)
    f.setStyleSheet(f"background: {C_BORDER}; border: none; max-height: 1px;")
    return f


def micro_label(text):
    l = QLabel(text)
    l.setStyleSheet(f"color: {C_TEXT2}; font-family: 'Courier New'; font-size: 9px;")
    l.setContentsMargins(6, 0, 0, 0)
    return l


def make_tab_btn(text):
    b = QPushButton(text)
    b.setCheckable(True)
    b.setStyleSheet(f"""
        QPushButton {{
            background: #1e2436; color: {C_TEXT2};
            border: none; border-radius: 4px;
            padding: 8px 12px;
            font-family: 'Courier New'; font-size: 11px;
            text-align: left;
        }}
        QPushButton:hover {{ background: #252d42; color: {C_TEXT1}; }}
        QPushButton:checked {{ background: {C_ACCENT}; color: #0d0f14; font-weight: bold; }}
    """)
    b.setCursor(Qt.CursorShape.PointingHandCursor)
    return b


def make_tool_btn(text):
    b = QPushButton(text)
    b.setCheckable(True)
    b.setStyleSheet(f"""
        QPushButton {{
            background: #1e2436; color: {C_TEXT2};
            border: 1px solid {C_BORDER}; border-radius: 4px;
            padding: 6px 12px;
            font-family: 'Courier New'; font-size: 11px;
        }}
        QPushButton:hover {{ background: #252d42; color: {C_TEXT1}; }}
        QPushButton:checked {{ background: {C_ACCENT2}; color: #fff; border-color: {C_ACCENT2}; }}
    """)
    b.setCursor(Qt.CursorShape.PointingHandCursor)
    return b


def make_action_btn(text, hover_bg=C_ACCENT2, hover_fg="#ffffff"):
    b = QPushButton(text)
    b.setStyleSheet(f"""
        QPushButton {{
            background: #1e2436; color: {C_TEXT2};
            border: none; border-radius: 4px;
            padding: 7px 12px;
            font-family: 'Courier New'; font-size: 11px;
            text-align: left;
        }}
        QPushButton:hover {{ background: {hover_bg}; color: {hover_fg}; }}
        QPushButton:pressed {{ background: {hover_bg}; color: {hover_fg}; opacity: 0.8; }}
    """)
    b.setCursor(Qt.CursorShape.PointingHandCursor)
    return b


def make_delete_btn():
    b = QPushButton("×")
    b.setFixedSize(20, 20)
    b.setStyleSheet(f"""
        QPushButton {{
            background: transparent; color: {C_TEXT2};
            border: none; border-radius: 3px;
            font-family: 'Courier New'; font-size: 13px; font-weight: bold;
            padding: 0;
        }}
        QPushButton:hover {{ background: {C_RED}; color: #fff; }}
    """)
    b.setCursor(Qt.CursorShape.PointingHandCursor)
    return b



class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Protogen MAX7219 Editor")
        self.setStyleSheet(f"background: {C_BG};")

        self.grids = {
            k: data_to_grid(v["data"], v["cols"], v["rows"])
            for k, v in PATTERNS.items()
        }
        self.current_key = "eye"
        self._undo_stack = []
        self._code_updating = False

        # Load saved presets from disk; build their grids too
        self._presets = load_presets()   # {id: {name,base,cols,rows,var,data}}
        for pid, p in self._presets.items():
            self.grids[pid] = data_to_grid(p["data"], p["cols"], p["rows"])
        self._build_ui()

        from PyQt6.QtGui import QShortcut, QKeySequence
        QShortcut(QKeySequence("Ctrl+Z"), self).activated.connect(self._undo)

        self._switch_pattern("eye")

    def _build_ui(self):
        root = QWidget()
        self.setCentralWidget(root)
        outer = QHBoxLayout(root)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.setSpacing(0)

        # ── Sidebar ────────────────────────────────────────────────────────
        sidebar = QWidget()
        sidebar.setFixedWidth(188)
        sidebar.setStyleSheet(f"background: {C_PANEL};")
        sl = QVBoxLayout(sidebar)
        sl.setContentsMargins(12, 22, 12, 22)
        sl.setSpacing(4)

        lbl_title = QLabel("PROTOGEN")
        lbl_title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lbl_title.setStyleSheet(
            f"color: {C_ACCENT}; font-family: 'Courier New'; font-size: 17px; font-weight: bold;")
        sl.addWidget(lbl_title)

        lbl_sub = QLabel("MAX7219 EDITOR")
        lbl_sub.setAlignment(Qt.AlignmentFlag.AlignCenter)
        lbl_sub.setStyleSheet(
            f"color: {C_TEXT2}; font-family: 'Courier New'; font-size: 9px;")
        sl.addWidget(lbl_sub)

        sl.addSpacing(12)
        sl.addWidget(divider())
        sl.addSpacing(8)
        sl.addWidget(micro_label("BASE PATTERNS"))
        sl.addSpacing(4)

        self.tab_group = QButtonGroup(self)
        self.tab_group.setExclusive(True)
        self.tab_btns = {}
        for key in PATTERNS:
            b = make_tab_btn(key.upper())
            self.tab_btns[key] = b
            self.tab_group.addButton(b)
            sl.addWidget(b)
            b.clicked.connect(lambda _, k=key: self._switch_pattern(k))

        sl.addSpacing(8)
        sl.addWidget(divider())
        sl.addSpacing(6)

        # Presets header row
        preset_hdr = QHBoxLayout()
        preset_hdr.setContentsMargins(6, 0, 0, 0)
        preset_hdr.addWidget(micro_label("PRESETS"))
        preset_hdr.addStretch()
        new_preset_btn = QPushButton("+ NEW")
        new_preset_btn.setFixedSize(46, 18)
        new_preset_btn.setStyleSheet(f"""
            QPushButton {{
                background: #1e2436; color: {C_ACCENT};
                border: 1px solid {C_ACCENT}; border-radius: 3px;
                font-family: 'Courier New'; font-size: 8px; font-weight: bold;
            }}
            QPushButton:hover {{ background: {C_ACCENT}; color: #0d0f14; }}
        """)
        new_preset_btn.clicked.connect(self._new_preset)
        new_preset_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        preset_hdr.addWidget(new_preset_btn)
        sl.addLayout(preset_hdr)
        sl.addSpacing(4)

        # Scrollable preset list
        self._preset_scroll = QScrollArea()
        self._preset_scroll.setWidgetResizable(True)
        self._preset_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self._preset_scroll.setStyleSheet(f"""
            QScrollArea {{ background: transparent; border: none; }}
            QScrollBar:vertical {{
                background: {C_PANEL}; width: 4px; border: none;
            }}
            QScrollBar::handle:vertical {{
                background: {C_BORDER}; border-radius: 2px;
            }}
        """)
        self._preset_list_widget = QWidget()
        self._preset_list_widget.setStyleSheet(f"background: transparent;")
        self._preset_list_layout = QVBoxLayout(self._preset_list_widget)
        self._preset_list_layout.setContentsMargins(0, 0, 0, 0)
        self._preset_list_layout.setSpacing(3)
        self._preset_list_layout.addStretch()
        self._preset_scroll.setWidget(self._preset_list_widget)
        self._preset_scroll.setMinimumHeight(60)
        self._preset_scroll.setMaximumHeight(200)
        sl.addWidget(self._preset_scroll)

        # Populate any presets loaded from disk
        for pid in list(self._presets.keys()):
            self._add_preset_row(pid)

        sl.addSpacing(8)
        sl.addWidget(divider())
        sl.addSpacing(8)
        sl.addWidget(micro_label("TOOL"))
        sl.addSpacing(4)

        self.tool_group = QButtonGroup(self)
        self.tool_group.setExclusive(True)
        self.paint_btn = make_tool_btn("● PAINT")
        self.erase_btn = make_tool_btn("○ ERASE")
        self.paint_btn.setChecked(True)
        for b in (self.paint_btn, self.erase_btn):
            self.tool_group.addButton(b)
            sl.addWidget(b)
        self.paint_btn.clicked.connect(lambda: self._set_tool("paint"))
        self.erase_btn.clicked.connect(lambda: self._set_tool("erase"))

        sl.addSpacing(8)
        sl.addWidget(divider())
        sl.addSpacing(8)
        sl.addWidget(micro_label("ACTIONS"))
        sl.addSpacing(4)

        for label, fn, hbg, hfg in [
            ("CLEAR ALL", self._clear,  C_RED,     "#0d0f14"),
            ("FILL ALL",  self._fill,   C_ACCENT,  "#0d0f14"),
            ("FLIP H",    self._flip_h, C_ACCENT2, "#ffffff"),
            ("FLIP V",    self._flip_v, C_ACCENT2, "#ffffff"),
            ("INVERT",    self._invert, C_ACCENT2, "#ffffff"),
        ]:
            b = make_action_btn(label, hover_bg=hbg, hover_fg=hfg)
            b.clicked.connect(fn)
            sl.addWidget(b)

        sl.addStretch()
        outer.addWidget(sidebar)

        # vertical divider line
        vline = QFrame()
        vline.setFrameShape(QFrame.Shape.VLine)
        vline.setStyleSheet(f"background: {C_BORDER}; border: none; max-width: 1px;")
        outer.addWidget(vline)

        # ── Main area ──────────────────────────────────────────────────────
        main_w = QWidget()
        main_w.setStyleSheet(f"background: {C_BG};")
        ml = QVBoxLayout(main_w)
        ml.setContentsMargins(24, 20, 24, 20)
        ml.setSpacing(10)

        # Pattern title row
        hdr = QHBoxLayout()
        self.pat_label = QLabel("")
        self.pat_label.setStyleSheet(
            f"color: {C_TEXT1}; font-family: 'Courier New'; font-size: 14px; font-weight: bold;")
        self.dim_label = QLabel("")
        self.dim_label.setStyleSheet(
            f"color: {C_TEXT2}; font-family: 'Courier New'; font-size: 11px;")
        hdr.addWidget(self.pat_label)
        hdr.addWidget(self.dim_label)
        hdr.addStretch()
        ml.addLayout(hdr)

        # Canvas card
        canvas_card = QFrame()
        canvas_card.setStyleSheet(
            f"background: {C_CARD}; border: 1px solid {C_BORDER}; border-radius: 6px;")
        ccl = QVBoxLayout(canvas_card)
        ccl.setContentsMargins(14, 14, 14, 14)
        self.canvas = PixelCanvas()
        self.canvas.changed.connect(self._on_canvas_changed)
        self.canvas.press_started.connect(self._push_undo)
        ccl.addWidget(self.canvas, alignment=Qt.AlignmentFlag.AlignLeft)
        ml.addWidget(canvas_card, alignment=Qt.AlignmentFlag.AlignLeft)

        # Code output header
        code_hdr = QHBoxLayout()
        code_hdr.addWidget(micro_label("C ARRAY  —  live editable"))
        code_hdr.addStretch()

        self.undo_btn = QPushButton("⟲ UNDO")
        self.undo_btn.setFixedSize(72, 22)
        self.undo_btn.setStyleSheet(f"""
            QPushButton {{
                background: #1e2436; color: {C_TEXT2};
                border: 1px solid {C_BORDER}; border-radius: 3px;
                font-family: 'Courier New'; font-size: 9px;
            }}
            QPushButton:hover {{ background: {C_ACCENT2}; color: #fff; border-color: {C_ACCENT2}; }}
            QPushButton:disabled {{ color: #333a52; border-color: #1e2436; }}
        """)
        self.undo_btn.clicked.connect(self._undo)
        self.undo_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        self.undo_btn.setEnabled(False)
        code_hdr.addWidget(self.undo_btn)

        copy_btn = QPushButton("COPY")
        copy_btn.setFixedSize(58, 22)
        copy_btn.setStyleSheet(f"""
            QPushButton {{
                background: #1e2436; color: {C_TEXT2};
                border: 1px solid {C_BORDER}; border-radius: 3px;
                font-family: 'Courier New'; font-size: 9px;
            }}
            QPushButton:hover {{ background: {C_ACCENT2}; color: #fff; border-color: {C_ACCENT2}; }}
        """)
        copy_btn.clicked.connect(self._copy_code)
        copy_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        code_hdr.addWidget(copy_btn)
        ml.addLayout(code_hdr)

        # Code box
        code_card = QFrame()
        code_card.setStyleSheet(
            f"background: {C_CARD}; border: 1px solid {C_BORDER}; border-radius: 6px;")
        ccl2 = QVBoxLayout(code_card)
        ccl2.setContentsMargins(14, 10, 14, 10)
        self.code_box = QTextEdit()
        self.code_box.setFont(QFont("Courier New", 11))
        self.code_box.setStyleSheet(f"""
            QTextEdit {{
                background: transparent;
                color: {C_ACCENT};
                border: none;
                selection-background-color: {C_ACCENT2};
            }}
            QScrollBar:vertical {{
                background: {C_CARD}; width: 8px; border: none;
            }}
            QScrollBar::handle:vertical {{
                background: {C_BORDER}; border-radius: 4px;
            }}
        """)
        self.code_box.setFixedHeight(130)
        self.code_box.textChanged.connect(self._on_code_edited)
        ccl2.addWidget(self.code_box)
        ml.addWidget(code_card)
        ml.addStretch()

        outer.addWidget(main_w)

    # ── Logic ─────────────────────────────────────────────────────────────────

    def _switch_pattern(self, key):
        # Save canvas state back before switching (only after first real load)
        if hasattr(self, "_canvas_loaded") and self._canvas_loaded:
            self.grids[self.current_key] = self.canvas.get_grid()
            # If current was a preset, persist the live grid to its data
            if self.current_key in self._presets:
                p = self._presets[self.current_key]
                p["data"] = grid_to_data(self.grids[self.current_key], p["cols"], p["rows"])

        self.current_key = key

        # Resolve metadata from base PATTERNS or _presets
        if key in PATTERNS:
            pat = PATTERNS[key]
            cols, rows = pat["cols"], pat["rows"]
            var = pat["var"]
            display = key.upper()
            self.tab_btns[key].setChecked(True)
        else:
            p = self._presets[key]
            cols, rows = p["cols"], p["rows"]
            var = p["var"]
            display = p["name"]
            self.tab_btns[key].setChecked(True)

        self.pat_label.setText(display)
        self.dim_label.setText(f"  {cols} × {rows}  matrix")
        self.canvas.load(cols, rows, self.grids[key])
        self._canvas_loaded = True
        self._update_code()

    def _active_var(self):
        """Return the C variable name string for whatever is currently selected."""
        if self.current_key in PATTERNS:
            return PATTERNS[self.current_key]["var"]
        return self._presets[self.current_key]["var"]

    def _active_dims(self):
        if self.current_key in PATTERNS:
            p = PATTERNS[self.current_key]
        else:
            p = self._presets[self.current_key]
        return p["cols"], p["rows"]

    # ── Preset management ─────────────────────────────────────────────────────

    def _new_preset(self):
        """Clone current pattern into a new named preset."""
        # Choose which base to clone from
        if self.current_key in PATTERNS:
            base_key = self.current_key
        else:
            base_key = self._presets[self.current_key]["base"]

        base_pat = PATTERNS[base_key]

        # Ask for a name
        name, ok = QInputDialog.getText(
            self, "New Preset", "Preset name:",
            text=f"custom_{base_key}"
        )
        if not ok or not name.strip():
            return
        name = name.strip()

        # Build a safe C identifier for the var name
        safe = re.sub(r'[^a-zA-Z0-9_]', '_', name)
        var = f"uint8_t {safe}[{base_pat['cols']}]"

        pid = new_preset_id(self._presets)

        # Clone current live grid (not the default data)
        current_grid = copy.deepcopy(self.grids[self.current_key])
        data = grid_to_data(current_grid, base_pat["cols"], base_pat["rows"])

        self._presets[pid] = {
            "name":  name,
            "base":  base_key,
            "cols":  base_pat["cols"],
            "rows":  base_pat["rows"],
            "var":   var,
            "data":  data,
        }
        self.grids[pid] = copy.deepcopy(current_grid)
        save_presets(self._presets)

        self._add_preset_row(pid)
        self._switch_pattern(pid)

    def _add_preset_row(self, pid):
        """Add a single preset row (tab btn + delete btn) to the scroll list."""
        p = self._presets[pid]

        row = QWidget()
        row.setStyleSheet("background: transparent;")
        rl = QHBoxLayout(row)
        rl.setContentsMargins(0, 0, 0, 0)
        rl.setSpacing(3)

        # Tab button — takes most of the width
        b = make_tab_btn(p["name"])
        b.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self.tab_btns[pid] = b
        self.tab_group.addButton(b)
        b.clicked.connect(lambda _, k=pid: self._switch_pattern(k))
        rl.addWidget(b)

        # Delete button
        del_b = make_delete_btn()
        del_b.clicked.connect(lambda _, k=pid, r=row: self._delete_preset(k, r))
        rl.addWidget(del_b)

        # Insert before the stretch at the end
        layout = self._preset_list_layout
        layout.insertWidget(layout.count() - 1, row)

    def _delete_preset(self, pid, row_widget):
        if pid not in self._presets:
            return
        # If this preset is currently active, fall back to eye
        if self.current_key == pid:
            self._canvas_loaded = False   # prevent stale save
            self._switch_pattern("eye")
        self.tab_group.removeButton(self.tab_btns.pop(pid))
        del self._presets[pid]
        if pid in self.grids:
            del self.grids[pid]
        save_presets(self._presets)
        row_widget.setParent(None)
        row_widget.deleteLater()

    def _save_preset(self):
        """Persist the current preset's live grid back to disk."""
        if self.current_key not in self._presets:
            return
        p = self._presets[self.current_key]
        self.grids[self.current_key] = self.canvas.get_grid()
        p["data"] = grid_to_data(self.grids[self.current_key], p["cols"], p["rows"])
        save_presets(self._presets)

    def _push_undo(self):
        """Snapshot the current grid for the active pattern onto the undo stack."""
        snapshot = copy.deepcopy(self.grids[self.current_key])
        self._undo_stack.append((self.current_key, snapshot))
        if len(self._undo_stack) > 100:
            self._undo_stack.pop(0)
        self.undo_btn.setEnabled(True)

    def _undo(self):
        if not self._undo_stack:
            return
        key, snapshot = self._undo_stack.pop()
        if key != self.current_key:
            self.current_key = key
            self.tab_btns[key].setChecked(True)
        self.grids[key] = snapshot
        if key in PATTERNS:
            cols, rows = PATTERNS[key]["cols"], PATTERNS[key]["rows"]
        else:
            cols, rows = self._presets[key]["cols"], self._presets[key]["rows"]
        self.canvas.load(cols, rows, snapshot)
        self._update_code()
        self.undo_btn.setEnabled(bool(self._undo_stack))

    def _on_code_edited(self):
        """Parse the text box and update the canvas if the hex values are valid."""
        if self._code_updating:
            return
        text = self.code_box.toPlainText()
        tokens = re.findall(r'0[xX][0-9A-Fa-f]{1,2}', text)
        cols, rows = self._active_dims()
        if len(tokens) != cols:
            self.code_box.setStyleSheet(self.code_box.styleSheet().replace(
                C_ACCENT, "#ff6b8a"))
            return
        self.code_box.setStyleSheet(self.code_box.styleSheet().replace(
            "#ff6b8a", C_ACCENT))
        data = [int(t, 16) for t in tokens]
        new_grid = data_to_grid(data, cols, rows)
        if new_grid == self.grids[self.current_key]:
            return
        self._push_undo()
        self.grids[self.current_key] = new_grid
        self.canvas.load(cols, rows, new_grid)
        # If editing a preset's code, persist immediately
        if self.current_key in self._presets:
            self._presets[self.current_key]["data"] = data
            save_presets(self._presets)

    def _set_tool(self, mode):
        self.canvas.tool = mode

    def _on_canvas_changed(self):
        self.grids[self.current_key] = self.canvas.get_grid()
        # Auto-save preset edits on every pixel change
        if self.current_key in self._presets:
            p = self._presets[self.current_key]
            p["data"] = grid_to_data(self.grids[self.current_key], p["cols"], p["rows"])
            save_presets(self._presets)
        self._update_code()

    def _clear(self):
        self._push_undo()
        cols, rows = self._active_dims()
        self.grids[self.current_key] = [[0]*cols for _ in range(rows)]
        self.canvas.load(cols, rows, self.grids[self.current_key])
        self._update_code()

    def _fill(self):
        self._push_undo()
        cols, rows = self._active_dims()
        self.grids[self.current_key] = [[1]*cols for _ in range(rows)]
        self.canvas.load(cols, rows, self.grids[self.current_key])
        self._update_code()

    def _flip_h(self):
        self._push_undo()
        g = self.canvas.get_grid()
        self.grids[self.current_key] = [list(reversed(r)) for r in g]
        cols, rows = self._active_dims()
        self.canvas.load(cols, rows, self.grids[self.current_key])
        self._update_code()

    def _flip_v(self):
        self._push_undo()
        g = self.canvas.get_grid()
        self.grids[self.current_key] = list(reversed(g))
        cols, rows = self._active_dims()
        self.canvas.load(cols, rows, self.grids[self.current_key])
        self._update_code()

    def _invert(self):
        self._push_undo()
        g = self.canvas.get_grid()
        self.grids[self.current_key] = [[1-v for v in row] for row in g]
        cols, rows = self._active_dims()
        self.canvas.load(cols, rows, self.grids[self.current_key])
        self._update_code()

    def _update_code(self):
        self._code_updating = True
        cols, rows = self._active_dims()
        data = grid_to_data(self.grids[self.current_key], cols, rows)
        cursor = self.code_box.textCursor()
        pos = cursor.position()
        self.code_box.setPlainText(fmt_array(data, self._active_var()))
        ss = self.code_box.styleSheet()
        if "#ff6b8a" in ss:
            self.code_box.setStyleSheet(ss.replace("#ff6b8a", C_ACCENT))
        cursor = self.code_box.textCursor()
        cursor.setPosition(min(pos, len(self.code_box.toPlainText())))
        self.code_box.setTextCursor(cursor)
        self._code_updating = False

    def _copy_code(self):
        QApplication.clipboard().setText(self.code_box.toPlainText())


# ── Entry ─────────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    app = QApplication(sys.argv)
    app.setStyle("Fusion")

    pal = QPalette()
    pal.setColor(QPalette.ColorRole.Window,      QColor(C_BG))
    pal.setColor(QPalette.ColorRole.WindowText,  QColor(C_TEXT1))
    pal.setColor(QPalette.ColorRole.Base,        QColor(C_CARD))
    pal.setColor(QPalette.ColorRole.Text,        QColor(C_TEXT1))
    pal.setColor(QPalette.ColorRole.Button,      QColor("#1e2436"))
    pal.setColor(QPalette.ColorRole.ButtonText,  QColor(C_TEXT1))
    app.setPalette(pal)

    win = MainWindow()
    win.resize(1060, 560)
    win.show()
    sys.exit(app.exec())