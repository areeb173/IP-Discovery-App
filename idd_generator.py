import math
import os
from datetime import date
from fpdf import FPDF
from fpdf.enums import WrapMode


def _build_context(ip_files):
    lines = []
    for i, f in enumerate(ip_files, 1):
        lines.append(f"[File {i}] {f.get('file_path', 'unknown')}")
        lines.append(f"  IP Type : {f.get('ip_type', 'unknown')}")
        lines.append(f"  Score   : {f.get('score', 0)}/100")
        lines.append(f"  Keywords: {', '.join(f.get('keywords', []))}")
        lines.append(f"  Summary : {f.get('summary', 'N/A')}")
        lines.append(f"  Reason  : {f.get('reasoning', 'N/A')}")
        lines.append("")
    return "\n".join(lines)


def generate_idd_sections(ip_files, query_fn):
    context = _build_context(ip_files)
    preamble = (
        "You are a patent attorney preparing an Invention Disclosure Document (IDD). "
        "The following are findings from an automated IP scan of source code.\n\n"
        f"{context}\n\n"
    )

    def ask(instruction):
        prompt = (
            preamble
            + instruction
            + "\n\nProvide only the requested content. Do not include section headers or labels."
        )
        return query_fn(prompt).strip()

    title = ask(
        "Write a concise, professional title for this invention (5–12 words, "
        "no ending punctuation)."
    )
    field = ask(
        "In 1–2 sentences, state the technical field this invention belongs to."
    )
    background = ask(
        "In 2–3 sentences, describe the problem or gap in prior art that "
        "this invention addresses."
    )
    summary = ask(
        "In 3–5 sentences, summarize the invention at a high level: what "
        "it does and how it achieves its goal."
    )
    description = ask(
        "In 4–8 sentences, describe in technical detail how the invention "
        "works, referencing the key components and logic visible in the "
        "scanned files."
    )
    claims = ask(
        "List 3–5 novel features or potential patent claims. "
        "Place each on its own line, starting with a dash (-)."
    )

    return {
        "title": title or "Untitled Invention",
        "field": field,
        "background": background,
        "summary": summary,
        "description": description,
        "claims": claims,
        "ip_files": ip_files,
        "date": date.today().isoformat(),
    }


# ---------------------------------------------------------------------------
# PDF rendering
# ---------------------------------------------------------------------------

def _safe(text):
    if not text:
        return ""
    return str(text).encode("latin-1", errors="replace").decode("latin-1")


def _fit_text(pdf, text, col_width):
    usable = col_width - 2 * pdf.c_margin
    text = _safe(text)
    if pdf.get_string_width(text) <= usable:
        return text
    suffix = "..."
    while text and pdf.get_string_width(text + suffix) > usable:
        text = text[:-1]
    return text + suffix


def build_pdf(idd_data):
    pdf = FPDF()
    pdf.set_margins(22, 22, 22)
    pdf.set_auto_page_break(auto=True, margin=22)
    pdf.add_page()

    content_width = pdf.w - 44

    pdf.set_font("Helvetica", "B", 8)
    pdf.set_text_color(130, 130, 130)
    pdf.cell(0, 5, "INVENTION DISCLOSURE DOCUMENT  |  CONFIDENTIAL", ln=True, align="C")
    pdf.ln(4)

    pdf.set_font("Helvetica", "B", 17)
    pdf.set_text_color(25, 25, 25)
    pdf.set_x(pdf.l_margin)
    pdf.multi_cell(pdf.epw, 10, _safe(idd_data["title"]), align="C", wrapmode=WrapMode.CHAR)
    pdf.ln(3)


    pdf.set_font("Helvetica", "", 9)
    pdf.set_text_color(110, 110, 110)
    pdf.cell(0, 6, f"Date of Conception: {idd_data['date']}", ln=True, align="C")
    pdf.ln(5)


    y = pdf.get_y()
    pdf.set_draw_color(190, 185, 180)
    pdf.line(22, y, pdf.w - 22, y)
    pdf.ln(8)


    sections = [
        ("1. Field of the Invention", idd_data.get("field", "")),
        ("2. Background", idd_data.get("background", "")),
        ("3. Summary of the Invention", idd_data.get("summary", "")),
        ("4. Detailed Description", idd_data.get("description", "")),
        ("5. Novel Features and Potential Claims", idd_data.get("claims", "")),
    ]

    for heading, body in sections:
        pdf.set_font("Helvetica", "B", 11)
        pdf.set_text_color(35, 45, 85)
        pdf.cell(0, 7, _safe(heading), ln=True)
        pdf.ln(1)
        pdf.set_font("Helvetica", "", 10)
        pdf.set_text_color(35, 35, 35)
        for para in _safe(body).split("\n"):
            if para.strip():
                pdf.set_x(pdf.l_margin)
                pdf.multi_cell(pdf.epw, 6, para, wrapmode=WrapMode.CHAR)
            else:
                pdf.ln(2)
        pdf.ln(7)

    pdf.set_font("Helvetica", "B", 11)
    pdf.set_text_color(35, 45, 85)
    pdf.cell(0, 7, "6. Supporting Evidence", ln=True)
    pdf.ln(3)

    cw_file  = 55
    cw_type  = 24
    cw_score = 16
    cw_summ  = content_width - cw_file - cw_type - cw_score

    pdf.set_font("Helvetica", "B", 8)
    pdf.set_fill_color(218, 224, 240)
    pdf.set_text_color(20, 20, 20)
    pdf.set_draw_color(180, 180, 195)
    for label, w in [("File", cw_file), ("Type", cw_type), ("Score", cw_score), ("AI Summary", cw_summ)]:
        pdf.cell(w, 7, _fit_text(pdf, label, w), border=1, fill=True)
    pdf.ln()

    line_h = 5
    pdf.set_font("Helvetica", "", 8)
    for i, f in enumerate(idd_data["ip_files"]):
        fill = i % 2 == 1
        bg_color = (245, 246, 250) if fill else (255, 255, 255)
        pdf.set_fill_color(*bg_color)
        pdf.set_draw_color(180, 180, 195)

        filename = os.path.basename(f.get("file_path", ""))
        ip_type  = f.get("ip_type", "-")
        score    = str(f.get("score", "-"))
        summary  = _safe(f.get("summary", ""))

        # Calculate row height from how many lines the summary needs
        usable_summ = max(1.0, cw_summ - 2 * pdf.c_margin)
        n_lines = max(1, math.ceil(pdf.get_string_width(summary) / usable_summ))
        row_h = n_lines * line_h

        row_y = pdf.get_y()
        row_x = pdf.l_margin

        # Check for page break before drawing the row
        if row_y + row_h > pdf.h - pdf.b_margin:
            pdf.add_page()
            row_y = pdf.get_y()

        # Fixed-width cells (file, type, score)
        for val, w in [(filename, cw_file), (ip_type, cw_type), (score, cw_score)]:
            pdf.cell(w, row_h, _fit_text(pdf, val, w), border=1, fill=fill)

        # Summary cell: fill + border drawn as a rect, text via multi_cell
        summ_x = pdf.get_x()
        pdf.rect(summ_x, row_y, cw_summ, row_h, style="FD")
        pdf.set_xy(summ_x, row_y)
        pdf.multi_cell(cw_summ, line_h, summary, border=0, fill=False, wrapmode=WrapMode.CHAR)

        # Always advance to the start of the next row
        pdf.set_xy(row_x, row_y + row_h)


    pdf.ln(8)
    pdf.set_font("Helvetica", "I", 8)
    pdf.set_text_color(155, 155, 155)
    pdf.cell(
        0, 5,
        "Generated by IP Finder. Contents are preliminary and for internal review only.",
        ln=True,
        align="C",
    )

    return bytes(pdf.output())
