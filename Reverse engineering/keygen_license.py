import hashlib

def compute_license_from_hwid(hwid):
    if len(hwid) != 16:
        raise ValueError("HWID must be exactly 16 characters")
    md = hashlib.md5(hwid.encode('ascii')).digest()
    rev = md[::-1]  # reverse bytes
    return rev.hex()  # lowercase hex

if __name__ == "__main__":
    hwid = "E3060400FFFB8B0F"
    print("License:", compute_license_from_hwid(hwid))