#!/usr/bin/env python3
"""
    CPU/Memory/LA monitor for UNIX-based OS (control program)
    Copyright (C) 2016  Devaev Maxim <mdevaev@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""


import struct
import time

import argh


# =====
def get_cpu_percent():
    import psutil
    st = psutil.cpu_times_percent()
    user = st.user - st.guest
    nice = st.nice - st.guest_nice
    idle_all = st.idle + st.iowait
    system_all = st.system + st.irq + st.softirq
    virtual = st.guest + st.guest_nice
    total = max(1, user + nice + system_all + idle_all + st.steal + virtual)
    return (
        st.nice / total * 100
        + st.user / total * 100
        + system_all / total * 100
        + (st.steal + st.guest) / total * 100
    )


def get_local_stat():
    import psutil
    with open("/proc/loadavg") as loadavg_file:
        averages = loadavg_file.readline().split()[:3]
        averages = tuple( float(avg) * 10 for avg in averages )
        return {
            "cpu":  get_cpu_percent(),
            "mem":  psutil.virtual_memory().percent,
            "la1":  averages[0],
            "la5":  averages[1],
            "la15": averages[2],
        }


class QuantumCore:
    def __init__(self, device):
        import serial
        self._tty = serial.Serial(device, 115200)

    def send(self, cpu, mem, la1, la5, la15):
        self._send_command([10, cpu])
        self._send_command([11, mem])
        self._send_command([12, la1])
        self._send_command([13, la5])
        self._send_command([14, la15])

    def _send_command(self, items):
        items = list(items) + [0] * (9 - len(items))
        self._tty.write(struct.pack("<ccccccccc", *map(self._make_byte, items)))

    def _make_byte(self, value) :
        return bytes([int(value)])


# =====
@argh.arg("--device", default="/dev/ttyACM0")
@argh.arg("--interval", default=1, type=int)
def run_local(device=None, interval=None):
    qc = QuantumCore(device)
    while True:
        qc.send(**get_local_stat())
        time.sleep(interval)


@argh.arg("--url", default="http://localhost:8080")
@argh.arg("--device", default="/dev/ttyACM0")
@argh.arg("--interval", default=1, type=int)
def run_remote(url=None, device=None, interval=None):
    import requests
    import requests.exceptions

    qc = QuantumCore(device)
    while True:
        try:
            response = requests.get(url)
        except requests.exceptions.Timeout:
            raise
        if str(response.status_code)[0] != "5":  # 5xx
            response.raise_for_status()
        qc.send(**response.json())
        time.sleep(interval)


@argh.arg("--port", default=8080)
def run_server(port=None):
    import json
    import wsgiref.simple_server

    def application(env, start_response):
        start_response("200 OK", [("Content-Type", "application/json")])
        return [json.dumps(get_local_stat()).encode()]

    httpd = wsgiref.simple_server.make_server("", port, application)
    print("Serving HTTP on port {}...".format(port))

    httpd.serve_forever()


def main():
    parser = argh.ArghParser(description="Quantum Core -- hardware monitoring")
    parser.add_commands((
        run_local,
        run_remote,
        run_server,
    ))
    parser.dispatch()


if __name__ == "__main__":
    main()
