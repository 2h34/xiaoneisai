import json
import math
import os
import sys
from pathlib import Path

import pythoncom
import win32com.client


SW_DOC_ASSEMBLY = 2
SW_OPEN_SILENT = 1
SW_OPEN_READ_ONLY = 2


def as_list(value):
    if value is None:
        return []
    if isinstance(value, (list, tuple)):
        return list(value)
    return [value]


def safe(getter, default=None):
    try:
        return getter()
    except Exception:
        return default


def transform_data(component):
    transform = safe(lambda: component.Transform2)
    if transform is None:
        return None
    data = safe(lambda: transform.ArrayData)
    if data is None:
        return None
    values = [float(x) for x in as_list(data)]
    result = {"array_data": values}
    if len(values) >= 12:
        result["translation_m"] = values[9:12]
        result["translation_mm"] = [1000.0 * x for x in values[9:12]]
        result["scale"] = values[12] if len(values) > 12 else 1.0
    return result


def component_box(component):
    data = safe(lambda: component.GetBox())
    values = [float(x) for x in as_list(data)] if data is not None else []
    if len(values) != 6:
        return None
    return {
        "min_m": values[:3],
        "max_m": values[3:],
        "min_mm": [1000.0 * x for x in values[:3]],
        "max_mm": [1000.0 * x for x in values[3:]],
        "size_mm": [1000.0 * (values[i + 3] - values[i]) for i in range(3)],
    }


def walk_component(component, depth=0):
    name = safe(lambda: str(component.Name2), "")
    path = safe(lambda: str(component.GetPathName()), "")
    item = {
        "depth": depth,
        "name": name,
        "path": path,
        "referenced_configuration": safe(
            lambda: str(component.ReferencedConfiguration), ""
        ),
        "suppression": safe(lambda: int(component.GetSuppression())),
        "transform": transform_data(component),
        "box": component_box(component),
        "children": [],
    }
    children = safe(lambda: component.GetChildren())
    for child in as_list(children):
        if child is not None:
            item["children"].append(walk_component(child, depth + 1))
    return item


def walk_features(model):
    items = []
    feature = safe(lambda: model.FirstFeature())
    guard = 0
    while feature is not None and guard < 20000:
        guard += 1
        name = safe(lambda: str(feature.Name), "")
        kind = safe(lambda: str(feature.GetTypeName2()), "")
        items.append({"name": name, "type": kind})
        feature = safe(lambda: feature.GetNextFeature())
    return items


def mate_entity_data(entity):
    component = safe(lambda: entity.ReferenceComponent)
    reference = safe(lambda: entity.Reference)
    params = safe(lambda: entity.EntityParams)
    return {
        "component_name": safe(lambda: str(component.Name2), "") if component else "",
        "component_path": safe(lambda: str(component.GetPathName()), "") if component else "",
        "reference_type": safe(lambda: str(reference.GetType()), "") if reference else "",
        "entity_params": [float(x) for x in as_list(params)] if params is not None else [],
    }


def read_mates(assembly):
    count = safe(lambda: int(assembly.GetMateCount()), 0)
    items = []
    for index in range(count):
        mate = safe(lambda index=index: assembly.GetMate(index))
        if mate is None:
            continue
        entity_count = safe(lambda: int(mate.GetMateEntityCount()), 0)
        entities = []
        for entity_index in range(entity_count):
            entity = safe(lambda entity_index=entity_index: mate.MateEntity(entity_index))
            if entity is not None:
                entities.append(mate_entity_data(entity))
        items.append({
            "index": index,
            "name": safe(lambda: str(mate.Name), ""),
            "type": safe(lambda: int(mate.Type)),
            "alignment": safe(lambda: int(mate.Alignment)),
            "entities": entities,
        })
    return items


def open_doc(sw, path):
    options = SW_OPEN_SILENT | SW_OPEN_READ_ONLY
    errors = win32com.client.VARIANT(pythoncom.VT_BYREF | pythoncom.VT_I4, 0)
    warnings = win32com.client.VARIANT(pythoncom.VT_BYREF | pythoncom.VT_I4, 0)
    result = sw.OpenDoc6(
        path, SW_DOC_ASSEMBLY, options, "", errors, warnings
    )
    if isinstance(result, tuple):
        model = result[0]
        extras = list(result[1:])
    else:
        model = result
        extras = []
    extras.extend([{"errors": errors.value, "warnings": warnings.value}])
    return model, extras


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: sw_inspect_block_arm.py INPUT.SLDASM OUTPUT.json")

    source = os.path.abspath(sys.argv[1])
    output = Path(sys.argv[2]).resolve()
    if not os.path.isfile(source):
        raise FileNotFoundError(source)

    pythoncom.CoInitialize()
    sw = None
    model = None
    started_sw = False
    try:
        try:
            sw = win32com.client.GetActiveObject("SldWorks.Application")
        except Exception:
            sw = win32com.client.Dispatch("SldWorks.Application")
            started_sw = True
            sw.Visible = False

        model, open_extras = open_doc(sw, source)
        if model is None:
            raise RuntimeError(f"SolidWorks could not open assembly; extras={open_extras!r}")

        configuration_manager = model.ConfigurationManager
        configuration = configuration_manager.ActiveConfiguration
        root = configuration.GetRootComponent3(False)
        if root is None:
            raise RuntimeError("Assembly has no resolved root component")

        flat_components = safe(lambda: model.GetComponents(False), [])
        flat_component_items = []
        for component in as_list(flat_components):
            if component is not None:
                item = walk_component(component, 0)
                item["children"] = []
                flat_component_items.append(item)

        report = {
            "source": source,
            "opened_read_only": True,
            "solidworks_revision": safe(lambda: str(sw.RevisionNumber())),
            "document_title": safe(lambda: str(model.GetTitle())),
            "document_path": safe(lambda: str(model.GetPathName())),
            "configuration": safe(lambda: str(configuration.Name)),
            "open_extras": open_extras,
            "root_component": walk_component(root),
            "flat_components": flat_component_items,
            "mates": read_mates(model),
            "top_level_features": walk_features(model),
        }
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        print(json.dumps({
            "ok": True,
            "output": str(output),
            "solidworks_revision": report["solidworks_revision"],
            "document_title": report["document_title"],
        }, ensure_ascii=False))
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
