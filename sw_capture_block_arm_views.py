import json
import os
import sys
from pathlib import Path

import pythoncom
import win32com.client

from sw_inspect_block_arm import open_doc, safe


VIEWS = {
    "front": 1,
    "top": 5,
    "iso": 7,
}


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: sw_capture_block_arm_views.py INPUT.SLDASM OUTPUT_DIR")

    source = os.path.abspath(sys.argv[1])
    output_dir = Path(sys.argv[2]).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    pythoncom.CoInitialize()
    sw = None
    model = None
    started_sw = False
    results = []
    try:
        try:
            sw = win32com.client.GetActiveObject("SldWorks.Application")
        except Exception:
            sw = win32com.client.Dispatch("SldWorks.Application")
            started_sw = True

        sw.Visible = False
        model, extras = open_doc(sw, source)
        if model is None:
            raise RuntimeError(f"SolidWorks could not open assembly; extras={extras!r}")

        for name, view_id in VIEWS.items():
            output = output_dir / f"block_arm_{name}.png"
            shown = safe(lambda view_id=view_id: model.ShowNamedView2("", view_id), False)
            safe(lambda: model.ViewZoomtofit2())
            safe(lambda: model.GraphicsRedraw2())
            saved = safe(lambda output=output: model.SaveAs3(str(output), 0, 0), False)
            results.append({
                "view": name,
                "view_id": view_id,
                "shown": bool(shown),
                "saved": bool(saved),
                "path": str(output),
                "exists": output.is_file(),
                "bytes": output.stat().st_size if output.is_file() else 0,
            })

        print(json.dumps({"open_extras": extras, "views": results}, ensure_ascii=False))
    finally:
        if sw is not None and model is not None:
            title = safe(lambda: str(model.GetTitle()))
            if title:
                safe(lambda: sw.CloseDoc(title))
        if sw is not None and started_sw:
            safe(lambda: sw.ExitApp())
        pythoncom.CoUninitialize()


if __name__ == "__main__":
    main()
