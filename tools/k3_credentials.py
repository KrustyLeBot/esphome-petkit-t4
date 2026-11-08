#!/usr/bin/env python3
"""Print the Bluetooth MAC and secret of the PETKIT K3 smart spray, as secrets.yaml lines.

The K3 only obeys a box that authenticates with the K3's secret, and only the Petkit cloud
knows it. Run this ONCE, while the Petkit account still exists, then put the two lines it
prints in secrets.yaml (k3_mac, k3_secret). Nothing is written to disk or sent anywhere but
to Petkit's own servers (the same login the Petkit app does).

The output is a credential: do not paste it in an issue, a chat or a commit.

Setup (any machine with Python 3.11+, e.g. WSL):
    python3 -m venv ~/k3env
    ~/k3env/bin/pip install pypetkitapi==1.31.1
    ~/k3env/bin/python tools/k3_credentials.py

Uses Jezza34000/py-petkit-api (pypetkitapi). If the account asks for an email code instead of
the password, the script requests the code and asks for it.
"""

import asyncio
import getpass
import re
import sys

try:
    import aiohttp
    from pypetkitapi import PetKitClient
    from pypetkitapi.exceptions import PypetkitError
    from pypetkitapi.purifier_container import Purifier
except ImportError:
    sys.exit("pypetkitapi is missing: ~/k3env/bin/pip install pypetkitapi==1.31.1")


def as_mac(value):
    """'a4c1380f1e2d' or 'a4:c1:...' -> 'A4:C1:38:0F:1E:2D' (the form ESPHome wants)."""
    if not value:
        return None
    hexchars = re.sub(r"[^0-9a-fA-F]", "", str(value))
    if len(hexchars) != 12:
        return str(value)
    return ":".join(hexchars[i:i + 2] for i in range(0, 12, 2)).upper()


async def login(client):
    try:
        await client.login()
        return
    except PypetkitError as err:
        print(f"Password login refused ({err}).")
    answer = input("Send a login code to the account's email instead? [y/N] ").strip().lower()
    if answer != "y":
        sys.exit("Stopped.")
    if not await client.request_login_code():
        sys.exit("Petkit did not send a code.")
    code = input("Code received by email: ").strip()
    await client.login(valid_code=code)


async def main():
    username = input("Petkit account (email): ").strip()
    password = getpass.getpass("Petkit password (not shown): ")
    region = input("Account region [FR]: ").strip() or "FR"

    async with aiohttp.ClientSession() as session:
        client = PetKitClient(username, password, region, "Europe/Paris", session=session)
        await login(client)
        await client.get_devices_data()

        purifiers = [d for d in client.petkit_entities.values() if isinstance(d, Purifier)]
        sprays = [d for d in purifiers
                  if (getattr(getattr(d, "device_nfo", None), "device_type", "") or "").lower() == "k3"]
        sprays = sprays or purifiers  # device type missing: show every spray/purifier rather than nothing
        if not sprays:
            kinds = sorted({type(d).__name__ for d in client.petkit_entities.values()})
            sys.exit(f"No K3 on this account (found: {', '.join(kinds) or 'nothing'}).")

        for k3 in sprays:
            mac = as_mac(k3.mac)
            bt_mac = as_mac(k3.bt_mac)
            print()
            print(f"# K3 \"{k3.name}\" (firmware {k3.firmware}, battery {k3.battery}, liquid {k3.liquid})")
            if bt_mac and bt_mac != mac:
                # The HA integration this protocol comes from connects to `mac`.
                print(f"# btMac differs: {bt_mac} — try it as k3_mac if the box cannot find the K3.")
            print(f"k3_mac: '{mac}'")
            print(f"k3_secret: '{k3.secret or ''}'")
            if not k3.secret:
                print("# No secret returned: the K3 may not be bound to this account any more.")
        print()
        print("Copy the two k3_ lines into secrets.yaml. Keep them private.")


if __name__ == "__main__":
    asyncio.run(main())
