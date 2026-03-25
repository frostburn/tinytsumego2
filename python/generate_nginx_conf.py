from dotenv import load_dotenv
import ctypes
from ctypes import pointer
from lib_types import *
from lib_defs import lib
import os
import sys

libc = ctypes.CDLL("libc.so.6")

Version = ctypes.c_char * 16
version_str = Version()
lib.version(version_str)
version = version_str.value.decode()

if __name__ == "__main__":
    print(f"Running TinyTsumego v{version}")

    load_dotenv()
    COLLECTION_PATH = os.getenv("COLLECTION_PATH")
    if COLLECTION_PATH is NONE:
        sys.stderr.write("COLLECTION_PATH not found in .env\n")
        sys.exit(1)

    if len(sys.argv) > 1:
        output_path = sys.argv[1]
    else:
        sys.stderr.write("Output path must be provided\n")
        sys.exit(1)

    binary_slugs = set()

    for filename in os.listdir(COLLECTION_PATH):
        [slug, ext] = os.path.splitext(filename)
        if ext != ".bin":
            continue
        binary_slugs.add(slug)

    slugs = []
    num_collections = ctypes.c_int(0)
    pc = lib.get_collections(pointer(num_collections))
    for i in range(num_collections.value):
        slug = pc[i].slug.decode()
        if slug in binary_slugs:
            print("Configuring upstream for", slug)
            slugs.append(slug)
        else:
            print("Missing binary for", slug)
            slugs.append(None)

    print("Cleaning up...")
    for i in range(num_collections.value):
        libc.free(pc[i].tsumegos)
    libc.free(pc)

    port = 8400
    with open(output_path, "w") as f:
        for slug in slugs:
            if slug is not None:
                f.write(f"location /api/tsumego/{slug}/ {{\n")
                f.write(f"  proxy_set_header Host $host;\n")
                f.write(f"  proxy_pass http://127.0.0.1:{port}/;\n")
                f.write(f"}}\n")
            port += 1
