import os
import struct
import sys

PROXIES = ["version", "winmm", "dinput8"]
WINDOWS = os.environ.get("SystemRoot", r"C:\Windows")
FOLDERS = {"x64": ("System32", "Sysnative"), "x86": ("SysWOW64", "SysWOW64")}


def read_exports(path):
    with open(path, "rb") as file:
        data = file.read()
    nt = struct.unpack_from("<I", data, 0x3C)[0]
    if data[nt:nt + 4] != b"PE\0\0":
        raise ValueError(f"{path} is not a PE file")
    sections = struct.unpack_from("<H", data, nt + 6)[0]
    optional_size = struct.unpack_from("<H", data, nt + 20)[0]
    optional = nt + 24
    magic = struct.unpack_from("<H", data, optional)[0]
    directories = optional + (96 if magic == 0x10B else 112)
    export_rva, export_size = struct.unpack_from("<II", data, directories)
    section_table = optional + optional_size

    def offset(rva):
        for index in range(sections):
            entry = section_table + index * 40
            virtual_size, virtual_address, raw_size, raw_pointer = struct.unpack_from("<IIII", data, entry + 8)
            if virtual_address <= rva < virtual_address + max(virtual_size, raw_size):
                return rva - virtual_address + raw_pointer
        raise ValueError(f"rva {rva:#x} is outside every section")

    def string(rva):
        start = offset(rva)
        return data[start:data.index(b"\0", start)].decode("ascii")

    directory = offset(export_rva)
    base, function_count, name_count, functions, names, ordinals = struct.unpack_from("<IIIIII", data, directory + 16)
    named = {}
    for index in range(name_count):
        name = string(struct.unpack_from("<I", data, offset(names) + index * 4)[0])
        named[struct.unpack_from("<H", data, offset(ordinals) + index * 2)[0]] = name
    exports = []
    for index in range(function_count):
        if struct.unpack_from("<I", data, offset(functions) + index * 4)[0] == 0:
            continue
        exports.append((base + index, named.get(index)))
    return exports


def directive(proxy, folder, ordinal, name):
    target = f"\\\\\\\\.\\\\GLOBALROOT\\\\SystemRoot\\\\{folder}\\\\{proxy}."
    if name:
        return f'#pragma comment(linker, "/EXPORT:{name}={target}{name},@{ordinal}")'
    return f'#pragma comment(linker, "/EXPORT:OmegaWareOrdinal{ordinal}={target}#{ordinal},@{ordinal},NONAME")'


def generate(proxy, output_folder):
    lines = []
    for arch, condition in (("x64", "#ifdef _WIN64"), ("x86", "#else")):
        folder, read_folder = FOLDERS[arch]
        source = os.path.join(WINDOWS, read_folder, f"{proxy}.dll")
        if not os.path.exists(source):
            source = os.path.join(WINDOWS, folder, f"{proxy}.dll")
        exports = read_exports(source)
        lines.append(condition)
        lines.extend(directive(proxy, folder, ordinal, name) for ordinal, name in exports)
        print(f"{proxy}.dll {arch}: {len(exports)} exports from {source}")
    lines.append("#endif")
    with open(os.path.join(output_folder, f"{proxy}.cpp"), "w", encoding="utf-8", newline="\n") as file:
        file.write("\n".join(lines) + "\n")


def main():
    output_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Exports")
    os.makedirs(output_folder, exist_ok=True)
    for proxy in sys.argv[1:] or PROXIES:
        generate(proxy, output_folder)


if __name__ == "__main__":
    main()
