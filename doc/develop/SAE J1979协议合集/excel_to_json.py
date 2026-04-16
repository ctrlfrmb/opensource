#!/usr/bin/env python3
"""
Export OBD-II Mode 01 PIDs from J1979DA_201702.xlsx (Annex B) to JSON.
Fixes: formula, unit, enum type, data_bytes, byte variable reference, signed.

Usage: python excel_to_json.py
Input:  J1979DA_201702.xlsx (same directory)
Output: config/obd/obd_mode01_pids_XX_YY.json
"""
import openpyxl
import json
import re
import os
import sys

OUTPUT_DIR = "config/obd"
os.makedirs(OUTPUT_DIR, exist_ok=True)

# ── Simple log helper ──
VERBOSE = ("-v" in sys.argv or "--verbose" in sys.argv)

def log_info(msg):
    print(f"  INFO  {msg}")

def log_warn(msg):
    print(f"  WARN  {msg}")

def log_debug(msg):
    if VERBOSE:
        print(f"  DEBUG {msg}")

# ── SI → Imperial alt-unit conversion table ──
ALT_UNITS = {
    "°C":    ("°F",     1.8,       32),
    "kPa":   ("PSI",    0.14504,   0),
    "km/h":  ("mph",    0.6214,    0),
    "km":    ("miles",  0.6214,    0),
    "g/s":   ("lb/min", 0.13228,   0),
    "kg/h":  ("lb/h",   2.20462,   0),
    "Pa":    ("inH2O",  0.00401865,0),
    "L/h":   ("gal/h",  0.264172,  0),
    "L":     ("gal",    0.264172,  0),
    "Nm":    ("ft·lb",  0.7376,    0),
}

# ── Unit pattern → canonical unit (order matters: longer patterns first) ──
UNIT_PATTERNS = [
    (r'mg/m\s*[³3]',     'mg/m³'),
    (r'mg/stro?ke',       'mg/stroke'),
    (r'km/h',             'km/h'),
    (r'kg/h',             'kg/h'),
    (r'kPa',              'kPa'),
    (r'g/s',              'g/s'),
    (r'L/h',              'L/h'),
    (r'lb/min',           'lb/min'),
    (r'min[\-−]1|rpm',    'rpm'),
    (r'°C',               '°C'),
    (r'°F',               '°F'),
    (r'inHg',             'inHg'),
    (r'in\s*H2O',         'inH2O'),
    (r'PSI',              'PSI'),
    (r'mA\b',             'mA'),
    (r'ppm',              'ppm'),
    (r'Nm\b',             'Nm'),
    (r'Pa\b(?![a-z])',    'Pa'),
    (r'lambda',           'λ'),
    (r'\bsec\b',          'sec'),
    (r'\bmin\b',          'min'),
    (r'\bkm\b',           'km'),
    (r'\bh/bit\b',        'h'),
    (r'\bV\b',            'V'),
    (r'%',                '%'),
    (r'°',                '°'),
    (r'\bsecond',         'sec'),
]

# ────────────────── Helper Functions ──────────────────

def clean(val):
    if val is None:
        return ""
    return str(val).strip().replace('\n', ' ').replace('\r', '')

def safe_float(val, default=0.0):
    if val is None:
        return default
    try:
        return float(str(val).replace(',', '').replace('\u2013', '-').strip())
    except (ValueError, TypeError):
        return default

def build_var_expr(bytes_str):
    """Build big-endian variable expression.
    Handles standard (A,B) and extended (R,S,T,U / Z,A1,B1,C1) byte refs."""
    if not bytes_str:
        return "A"
    # Split by comma
    parts = [p.strip() for p in str(bytes_str).split(',')]
    parts = [p for p in parts if p and re.match(r'^[A-Z][0-9]?$', p, re.I)]
    if not parts:
        # Fallback: try single-letter extraction
        letters = re.findall(r'([A-Z])', bytes_str.upper())
        parts = letters if letters else ["A"]
    n = len(parts)
    if n == 1:
        return parts[0]
    if n == 2:
        return f"({parts[0]} * 256 + {parts[1]})"
    if n == 3:
        return f"({parts[0]} * 65536 + {parts[1]} * 256 + {parts[2]})"
    if n == 4:
        return f"({parts[0]} * 16777216 + {parts[1]} * 65536 + {parts[2]} * 256 + {parts[3]})"
    # >4 bytes: just use first 4
    return f"({parts[0]} * 16777216 + {parts[1]} * 65536 + {parts[2]} * 256 + {parts[3]})"

def max_byte_idx(bytes_str):
    """Compute max byte position index.
    A=0, B=1, ..., Z=25, A1=26, B1=27, ..."""
    if not bytes_str:
        return -1
    parts = [p.strip() for p in str(bytes_str).split(',')]
    max_idx = -1
    for p in parts:
        p = p.upper()
        m = re.match(r'^([A-Z])(\d?)$', p)
        if m:
            letter = m.group(1)
            suffix = m.group(2)
            idx = ord(letter) - ord('A')
            if suffix:
                idx += 26 * int(suffix)
            if idx > max_idx:
                max_idx = idx
    return max_idx

def extract_unit(scaling):
    if not scaling:
        return ""
    s = str(scaling)
    for pat, unit in UNIT_PATTERNS:
        if re.search(pat, s):
            return unit
    return ""

def display_fmt(display_str):
    if not display_str:
        return "%.2f"
    s = str(display_str)
    m = re.search(r'x+(\.)(x+)', s, re.IGNORECASE)
    if m:
        return f"%.{len(m.group(2))}f"
    if re.search(r'x{2,}', s, re.IGNORECASE):
        return "%.0f"
    return "%.2f"

def make_label(desc, max_len=12):
    if not desc:
        return ""
    words = desc.split()
    lbl = words[0].upper().rstrip(':,;.') if words else ""
    return lbl[:max_len]


# ════════════════════════════════════════════════════════
# Formula Parser
# ════════════════════════════════════════════════════════

def parse_formula(raw_scaling, bytes_str, min_val=None, max_val=None):
    """
    Parse 'Scaling/bit' column into formula string.
    Returns (formula_str, is_signed, unit_str).
    """
    if not raw_scaling:
        return ("", False, "")
    scaling = str(raw_scaling).strip()
    if not scaling or scaling.lower() == 'nan':
        return ("", False, "")

    sl = scaling.lower()

    # ── Non-formula types ──
    if 'bit mapped' in sl or 'bit-mapped' in sl:
        return ("Bit Mapped", False, "")
    if 'state encoded' in sl:
        return ("", False, "")
    if 'hexadecimal' in sl:
        return (build_var_expr(bytes_str), False, "")
    if sl.startswith('reserved') or sl.startswith('0 =') or sl.startswith('1 ='):
        return ("", False, "")
    if 'not' in sl and 'display' in sl:
        return ("", False, "")

    # ── Signed detection ──
    is_signed = 'signed' in sl

    # ── Replace Unicode fraction characters ──
    s = scaling
    s = s.replace('\u00BC', '0.25')   # ¼
    s = s.replace('\u00BD', '0.5')    # ½
    s = s.replace('\u00BE', '0.75')   # ¾

    var = build_var_expr(bytes_str)
    unit = extract_unit(scaling)

    # ── Extract factor ──
    factor = None
    frac_num = None
    frac_den = None

    # Pattern A: fraction  "100/255 % per bit", "100/128%"
    m = re.match(r'([\d.]+)\s*/\s*([\d.]+)', s)
    if m:
        frac_num = m.group(1)
        frac_den = m.group(2)
        factor = float(frac_num) / float(frac_den)

    # Pattern B: leading decimal "0.01 g/s", "3 kPa", "0.005 V", "0.0000305"
    if factor is None:
        m = re.match(r'^([\d.]+)\s', s)
        if m:
            f = float(m.group(1))
            if f != 0:
                factor = f

    # Pattern C: "1 xxx per bit"
    if factor is None and re.search(r'^1\s+\S+\s+(per\s+bit|/bit)', s, re.I):
        factor = 1.0

    # Pattern D: Enum code or text → not a formula
    if factor is None:
        sc = s.strip()
        if re.match(r'^[\$]?[0-9A-Fa-f]{1,4}(\s*[-–]\s*[\$]?[0-9A-Fa-f]+)?$', sc):
            return ("", False, "")
        if re.match(r'^[A-Za-z]', sc) and 'per bit' not in sl and '/bit' not in sl:
            return ("", False, "")

    if factor is None:
        factor = 1.0

    # ── Extract offset ──
    offset = 0.0

    # Pattern 1: "0% at 128", "0° at 26880"
    m_at = re.search(r'0[°%]?\s*at\s*([\d.]+)', s, re.I)
    if m_at:
        zero_point = float(m_at.group(1))
        offset = -zero_point * factor

    # Pattern 2: "with -40 °C offset"
    if offset == 0:
        m_off = re.search(r'with\s+([-+]?\d+(?:\.\d+)?)\s*(?:°|[A-Za-z]|offset|\s)', s, re.I)
        if m_off:
            offset = float(m_off.group(1))

    # Pattern 3: "-125 offset", "offset -40"
    if offset == 0:
        m_off = re.search(r'([-+]\d+(?:\.\d+)?)\s*offset', s, re.I)
        if m_off:
            offset = float(m_off.group(1))
        else:
            m_off = re.search(r'offset\s*([-+]?\d+(?:\.\d+)?)', s, re.I)
            if m_off:
                offset = float(m_off.group(1))

    # ── Build formula string ──
    if factor == 1.0:
        fpart = var
    elif frac_num and frac_den:
        fpart = f"{var} * {frac_num} / {frac_den}"
    elif factor == int(factor) and factor > 0:
        fpart = f"{var} * {int(factor)}"
    else:
        fpart = f"{var} * {factor:g}"

    if offset == 0:
        formula = fpart
    elif offset < 0:
        a = abs(offset)
        formula = f"{fpart} - {int(a) if a == int(a) else f'{a:g}'}"
    else:
        formula = f"{fpart} + {int(offset) if offset == int(offset) else f'{offset:g}'}"

    return (formula, is_signed, unit)


# ════════════════════════════════════════════════════════
# Bit / Data-byte Detection
# ════════════════════════════════════════════════════════

def has_bit_keyword(db):
    return bool(db) and 'bit' in str(db).lower()

def is_pure_bytes(db):
    """Accept A-Z and compound refs like A1, B1, C1."""
    if not db:
        return False
    s = str(db).strip()
    if has_bit_keyword(s):
        return False
    return bool(re.match(r'^[A-Z][0-9]?(\s*,\s*[A-Z][0-9]?)*\s*$', s, re.I))

def parse_bit_ref(db):
    s = str(db).strip()
    # Range
    m = re.search(r'([A-Q])?,?\s*bits?\s+(\d+)\s*[-–]\s*(\d+)', s, re.I)
    if m:
        byte_id = m.group(1).upper() if m.group(1) else None
        return byte_id, list(range(int(m.group(2)), int(m.group(3)) + 1))
    # Single
    m = re.search(r'([A-Q]),?\s*bit\s+(\d+)', s, re.I)
    if m:
        return m.group(1).upper(), int(m.group(2))
    # No byte letter
    m = re.search(r'bits?\s+(\d+)\s*[-–]\s*(\d+)', s, re.I)
    if m:
        return None, list(range(int(m.group(1)), int(m.group(2)) + 1))
    return None, None

def parse_v0_v1(scaling, display):
    """Extract v0/v1 values for a single-bit definition."""
    sc = clean(scaling)
    dp = clean(display).upper()

    # ── From display column ──
    if 'OFF' in dp and 'ON' in dp:
        return "OFF", "ON"
    if 'NO' in dp and 'YES' in dp:
        return "NO", "YES"
    if 'N/A' in dp:
        sc_upper = sc.upper()
        if 'COMPLETE' in sc_upper or 'READY' in sc_upper:
            return "Complete/N/A", "Not complete"

    # ── From scaling: parenthesized short names ──
    # "0 = Misfire monitor not supported (NO); 1 = Misfire monitor supported (YES)"
    m0 = re.search(r'0\s*=.*?\((\w+)\)', sc)
    m1 = re.search(r'1\s*=.*?\((\w+)\)', sc)
    if m0 and m1:
        return m0.group(1), m1.group(1)

    # ── From scaling: full text after "=" (no parentheses) ──
    # "0 = Spark ignition monitors supported\n1 = Compression ignition monitors supported"
    # "0 = MIL OFF; 1 = MIL ON"
    lines = re.split(r'[\n;]+', sc)
    v0_text = None
    v1_text = None
    for line in lines:
        line = line.strip()
        m = re.match(r'^0\s*=\s*(.+)', line)
        if m:
            v0_text = m.group(1).strip().rstrip('.')
        m = re.match(r'^1\s*=\s*(.+)', line)
        if m:
            v1_text = m.group(1).strip().rstrip('.')
    if v0_text and v1_text:
        # Truncate very long texts
        if len(v0_text) > 80:
            v0_text = v0_text[:80]
        if len(v1_text) > 80:
            v1_text = v1_text[:80]
        return v0_text, v1_text

    # ── Fallback: "1 = Bank 1 - Sensor 1 present at that location" (only v1) ──
    m1 = re.search(r'1\s*=\s*(.+?)(?:\n|$)', sc)
    if m1:
        return "0", m1.group(1).strip()[:80]

    return "0", "1"

def parse_enum_row(desc, scaling, comment):
    sc = scaling.strip()
    d = desc.strip()
    if not sc and not d:
        return None
    # Skip ranges
    if re.search(r'[0-9A-Fa-f]+\s*[-–]\s*[0-9A-Fa-f]+', sc):
        return None
    if re.match(r'^\$[0-9A-Fa-f]+\s*[-–]\s*\$[0-9A-Fa-f]+', d):
        return None
    if 'reserved' in sc.lower() and not re.match(r'^[0-9A-Fa-f]{1,2}$', sc):
        return None
    if 'not available for assignment' in sc.lower():
        return None
    if 'special meaning' in sc.lower():
        return None

    code = None
    name = None
    # Pattern 1: Code in scaling column ("1", "0A")
    if re.match(r'^[0-9A-Fa-f]{1,2}$', sc):
        code = int(sc, 16)
        name = d
    # Pattern 2: Code in desc as "$xx"
    if code is None:
        m = re.match(r'^\$([0-9A-Fa-f]{2})$', d)
        if m:
            code = int(m.group(1), 16)
            name = sc if sc else d
    if code is None:
        return None
    return (code, name or d, comment[:400] if comment else "")


# ════════════════════════════════════════════════════════
# Main Extraction
# ════════════════════════════════════════════════════════

def extract_mode01(wb):
    log_info("Loading Annex B - Parameter IDs...")
    sheet = wb['Annex B - Parameter IDs']

    pids = {}
    cur = None
    cur_num = None
    is_enum = False
    last_byte = 'A'

    def pid_tag():
        return f"PID ${cur_num:02X}" if cur_num is not None else "PID $??"

    def finalize(pid_dict):
        if pid_dict is None:
            return
        if pid_dict.get("type") == "supported":
            return
        mb = pid_dict.pop("_max_bi", -1)
        if mb >= 0:
            pid_dict["data_bytes"] = mb + 1
        if pid_dict.get("bits") and pid_dict.get("type") == "numeric":
            pid_dict["type"] = "bitmap"
        t = pid_dict.get("type", "")
        if t != "enum":
            if not pid_dict.get("enums"):
                pid_dict.pop("enums", None)
        if t == "enum":
            if not pid_dict.get("items"):
                pid_dict.pop("items", None)
            if not pid_dict.get("bits"):
                pid_dict.pop("bits", None)
            if not pid_dict.get("numeric_fields"):
                pid_dict.pop("numeric_fields", None)
        # Clean empty lists
        for key in ["items", "bits", "numeric_fields", "enums"]:
            if key in pid_dict and not pid_dict[key]:
                pass  # Keep empty arrays for consistency

    def update_max_bi(pd, bs):
        bi = max_byte_idx(bs)
        if bi > pd.get("_max_bi", -1):
            pd["_max_bi"] = bi

    row_count = 0
    for row in sheet.iter_rows(min_row=2, values_only=True):
        row_count += 1
        if not row or not any(row):
            continue

        c_pid  = clean(row[0])
        c_iso  = clean(row[1])
        c_desc = clean(row[2])
        c_db   = clean(row[3])
        c_min  = row[4]
        c_max  = row[5]
        c_scal = clean(row[6])
        c_disp = clean(row[7])
        c_comm = clean(row[8]) if len(row) > 8 else ""
        c_rst  = clean(row[9]) if len(row) > 9 else ""

        # ── Detect new PID ──
        row_pid_num = None
        if c_pid.startswith('0x') and '-' not in c_pid:
            try:
                row_pid_num = int(c_pid, 16)
            except ValueError:
                pass

        if row_pid_num is not None and row_pid_num != cur_num:
            if cur is not None:
                finalize(cur)
                pids[cur_num] = cur

            cur_num = row_pid_num
            is_enum = False
            last_byte = 'A'

            iso_num = 0
            if c_iso.startswith('0x'):
                try:
                    iso_num = int(c_iso.replace(' ', ''), 16)
                except ValueError:
                    pass

            if 'Defined in Appendix' in c_desc:
                cur = {"pid": cur_num, "iso_pid": iso_num,
                       "name": f"Supported PIDs [${cur_num+1:02X}-${cur_num+0x20:02X}]",
                       "name_zh": "", "type": "supported", "data_bytes": 4}
                log_debug(f"{pid_tag()}: Supported PID range")
                continue

            cur = {
                "pid": cur_num, "iso_pid": iso_num,
                "name": c_desc, "name_zh": "",
                "description": "", "description_zh": "",
                "data_bytes": 0, "type": "numeric",
                "items": [], "bits": [], "numeric_fields": [], "enums": [],
                "_max_bi": -1
            }
            if c_rst.strip().lower() in ('x', 'yes'):
                cur["reset_on_clear"] = True
            log_debug(f"{pid_tag()}: New PID → {c_desc}")

            # If header row also has data byte, fall through
            if not c_db:
                continue

        if cur is None or cur.get("type") == "supported":
            continue

        # ── Empty data byte → description capture ──
        if not c_db:
            if c_comm and not cur.get("description") and len(c_comm) > 20:
                cur["description"] = c_comm[:800]
                log_debug(f"{pid_tag()}: Captured description ({len(c_comm)} chars)")
            continue

        # ── Enum mode ──
        if is_enum:
            result = parse_enum_row(c_desc, c_scal, c_comm)
            if result:
                code, name, desc = result
                cur["enums"].append({
                    "code": code, "name": name, "name_zh": "",
                    "desc": desc, "desc_zh": ""
                })
                log_debug(f"{pid_tag()}: Enum code={code}, name={name[:40]}")
            continue

        # ── Detect STATE ENCODED VARIABLE (exact match, not partial like "Bit Mapped/State Encoded") ──
        if c_scal.strip().upper() == 'STATE ENCODED VARIABLE':
            cur["type"] = "enum"
            is_enum = True
            letters = re.findall(r'[A-Q]', c_db.upper())
            cur["enum_byte"] = letters[0] if letters else "A"
            update_max_bi(cur, c_db)
            log_info(f"{pid_tag()}: Enum type detected, byte={cur.get('enum_byte')}")
            continue

        # ── Bit field ──
        if has_bit_keyword(c_db):
            byte_id, bit_info = parse_bit_ref(c_db)
            if byte_id:
                last_byte = byte_id
            elif byte_id is None:
                byte_id = last_byte

            update_max_bi(cur, byte_id)

            if 'reserved' in c_desc.lower() or 'reserved' in c_scal.lower():
                continue

            if isinstance(bit_info, list):
                low, high = min(bit_info), max(bit_info)
                nbits = high - low + 1
                mask = (1 << nbits) - 1
                formula = f"({byte_id} >> {low}) & {mask}" if low > 0 else f"{byte_id} & {mask}"
                nf = {
                    "label": make_label(c_desc),
                    "name": c_desc, "name_zh": "",
                    "bytes": byte_id, "formula": formula,
                    "unit": "", "min": 0.0, "max": float(mask),
                    "display": "%.0f"
                }
                cur["numeric_fields"].append(nf)
                cur["type"] = "bitmap"
                log_debug(f"{pid_tag()}: NumericField {byte_id} bits {low}-{high}")

            elif isinstance(bit_info, int):
                v0, v1 = parse_v0_v1(c_scal, c_disp)
                cur["bits"].append({
                    "byte": byte_id, "bit": bit_info,
                    "name": c_desc, "name_zh": "",
                    "v0": v0, "v1": v1
                })
                cur["type"] = "bitmap"
                log_debug(f"{pid_tag()}: Bit {byte_id}.{bit_info} = {c_desc[:50]}")
            continue

        # ── Pure byte field: "A", "A,B" ──
        if is_pure_bytes(c_db):
            update_max_bi(cur, c_db)

            if 'bit mapped' in c_scal.lower():
                cur["items"].append({
                    "label": make_label(c_desc),
                    "name": c_desc, "name_zh": "",
                    "bytes": c_db.strip(),
                    "formula": "Bit Mapped",
                    "unit": "", "min": 0.0, "max": 0.0,
                    "display": ""
                })
                cur["type"] = "bitmap"
                log_debug(f"{pid_tag()}: Bitmap header: {c_db}")
                continue

            min_v = safe_float(c_min)
            max_v = safe_float(c_max)
            formula, is_signed, unit = parse_formula(c_scal, c_db, min_v, max_v)

            if not formula:
                cur["items"].append({
                    "label": make_label(c_desc),
                    "name": c_desc, "name_zh": "",
                    "bytes": c_db.strip(),
                    "formula": "",
                    "unit": "", "min": 0.0, "max": 0.0,
                    "display": ""
                })
                if c_comm and not cur.get("description") and len(c_comm) > 20:
                    cur["description"] = c_comm[:800]
                log_debug(f"{pid_tag()}: Group header: {c_db} → {c_desc[:40]}")
                continue

            if not unit:
                unit = extract_unit(c_scal)

            alt_unit, alt_factor, alt_offset = ALT_UNITS.get(unit, ("", 0, 0))
            fmt = display_fmt(c_disp)

			# Fix Excel percentage format: if unit is '%' and max ≤ 1 and formula implies 0-100 range
            if unit == '%' and 0 < max_v <= 1.0:
               max_v = max_v * 100
            if unit == '%' and 0 < min_v <= 1.0 and min_v != 0:
               min_v = min_v * 100

            item = {
                "label": make_label(c_desc),
                "name": c_desc, "name_zh": "",
                "bytes": c_db.strip(),
                "formula": formula,
                "unit": unit,
                "min": min_v, "max": max_v,
                "display": fmt
            }
            if is_signed:
                item["signed"] = True
            if alt_unit:
                item["unit_alt"] = alt_unit
                item["alt_factor"] = alt_factor
                item["alt_offset"] = alt_offset

            cur["items"].append(item)
            log_debug(f"{pid_tag()}: Item {c_db} → formula='{formula}', unit={unit}")

            if c_comm and not cur.get("description") and len(c_comm) > 20:
                cur["description"] = c_comm[:800]
            continue

        log_warn(f"{pid_tag()}: Unrecognized data byte: '{c_db}' (desc: {c_desc[:40]})")

    # Save last
    if cur is not None:
        finalize(cur)
        pids[cur_num] = cur

    log_info(f"Extracted {len(pids)} PIDs from {row_count} rows")
    return pids


# ════════════════════════════════════════════════════════
# JSON Output
# ════════════════════════════════════════════════════════

def save_mode01_chunks(pids):
    total = 0
    for cs in range(0x00, 0x100, 0x20):
        ce = cs + 0x1F
        chunk = [pids[p] for p in sorted(pids.keys()) if cs <= p <= ce]
        if not chunk:
            continue
        fname = f"obd_mode01_pids_{cs:02X}_{ce:02X}.json"
        fpath = os.path.join(OUTPUT_DIR, fname)
        with open(fpath, 'w', encoding='utf-8') as f:
            json.dump({"language": "en", "pids": chunk}, f, indent=2, ensure_ascii=False)
        total += 1
        log_info(f"Saved {fname} ({len(chunk)} PIDs, range ${cs:02X}-${ce:02X})")
    return total


# ════════════════════════════════════════════════════════
# Verification
# ════════════════════════════════════════════════════════

def verify(pids):
    print("\n" + "=" * 72)
    print("VERIFICATION SUMMARY")
    print("=" * 72)

    checks = [
        (0x04, "LOAD",   "A * 100 / 255",         "%"),
        (0x05, "ECT",    "A - 40",                 "°C"),
        (0x06, "SHRTFT", "A * 100 / 128 - 100",   "%"),
        (0x0A, "FP",     "A * 3",                  "kPa"),
        (0x0B, "MAP",    "A",                      "kPa"),
        (0x0C, "RPM",    "(A * 256 + B) * 0.25",  "rpm"),
        (0x0D, "VSS",    "A",                      "km/h"),
        (0x0E, "SPARK",  "A * 0.5 - 64",          "°"),
        (0x0F, "IAT",    "A - 40",                 "°C"),
        (0x10, "MAF",    "(A * 256 + B) * 0.01",  "g/s"),
        (0x11, "TP",     "A * 100 / 255",          "%"),
    ]

    for pid_num, label, exp_f, exp_u in checks:
        pid = pids.get(pid_num)
        if not pid or pid.get("type") == "supported":
            print(f"  PID ${pid_num:02X} {label:8s}: NOT FOUND")
            continue
        items = pid.get("items", [])
        if not items:
            print(f"  PID ${pid_num:02X} {label:8s}: NO ITEMS")
            continue
        it = items[0]
        f = it.get("formula", "")
        u = it.get("unit", "")
        ok_f = "✓" if f == exp_f else "✗"
        ok_u = "✓" if u == exp_u else "✗"
        status = "OK" if (ok_f == "✓" and ok_u == "✓") else "MISMATCH"
        print(f"  PID ${pid_num:02X} {label:8s}: [{status:8s}] "
              f"formula {ok_f} got='{f}' exp='{exp_f}'  "
              f"unit {ok_u} got='{u}' exp='{exp_u}'")

    # PID $06 Bank 3 (byte B)
    pid06 = pids.get(0x06)
    if pid06 and len(pid06.get("items", [])) >= 2:
        it1 = pid06["items"][1]
        f1 = it1.get("formula", "")
        ok = "✓" if "B" in f1 and "128" in f1 else "✗"
        print(f"  PID $06 BANK3   : [{('OK' if ok=='✓' else 'MISMATCH'):8s}] "
              f"formula {ok} got='{f1}' exp='B * 100 / 128 - 100'")

    # PID $1C enum
    pid1c = pids.get(0x1C)
    if pid1c:
        t = pid1c.get("type", "?")
        ne = len(pid1c.get("enums", []))
        ok = "✓" if t == "enum" and ne > 10 else "✗"
        print(f"  PID $1C OBD_REQ : [{('OK' if ok=='✓' else 'MISMATCH'):8s}] "
              f"type {ok} got='{t}', {ne} enum entries (exp 'enum', >10)")

    # PID $14 O2 + SHRTFT
    pid14 = pids.get(0x14)
    if pid14 and len(pid14.get("items", [])) >= 2:
        its = pid14["items"]
        f0 = its[0].get("formula", "")
        f1 = its[1].get("formula", "")
        ok0 = "✓" if ("0.005" in f0 or "/200" in f0) else "✗"
        ok1 = "✓" if ("B" in f1 and "128" in f1) else "✗"
        u0 = its[0].get("unit", "")
        u1 = its[1].get("unit", "")
        print(f"  PID $14 O2V     : [{('OK' if ok0=='✓' else 'MISMATCH'):8s}] "
              f"formula {ok0} got='{f0}' unit='{u0}'")
        print(f"  PID $14 SHRTFT  : [{('OK' if ok1=='✓' else 'MISMATCH'):8s}] "
              f"formula {ok1} got='{f1}' unit='{u1}'")

    # PID $1F Time since engine start
    pid1f = pids.get(0x1F)
    if pid1f and pid1f.get("items"):
        it = pid1f["items"][0]
        f = it.get("formula", "")
        u = it.get("unit", "")
        db = pid1f.get("data_bytes", 0)
        print(f"  PID $1F TIME    : formula='{f}' unit='{u}' data_bytes={db}")

    print("=" * 72)


# ════════════════════════════════════════════════════════
# Main
# ════════════════════════════════════════════════════════

def main():
    xlsx = 'J1979DA_201702.xlsx'
    if not os.path.exists(xlsx):
        print(f"ERROR: File not found: {xlsx}")
        return

    print(f"Loading {xlsx}...")
    wb = openpyxl.load_workbook(xlsx, read_only=True, data_only=True)

    pids = extract_mode01(wb)
    n = save_mode01_chunks(pids)

    verify(pids)

    print(f"\n✅ Done! {n} JSON files saved to {OUTPUT_DIR}/")
    print("Tip: Run with -v flag for verbose debug output")

if __name__ == "__main__":
    main()
