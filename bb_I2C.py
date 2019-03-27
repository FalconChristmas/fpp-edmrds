#!/usr/bin/env python

import time

import pigpio



BB_I2C_SCRIPT = (
    # v0 SCL, v1 SDA, v2 dly, v3 ack, v4 tx_byte
    " dcr p0"
    " jp 100"

    # command 0 (init)
    " ld v0 p1"  # save SCL
    " ld v1 p2"  # save SDA
    " ld v2 p3"  # save dly
    " halt"
    " tag 100"
    " dcr p0"
    " jp 200"

    # command 1 (start sequence)
    " call 1000"
    " halt"
    " tag 200"
    " dcr p0"
    " jp 300"

    # command 2 (stop sequence)
    " call 2000"
    " halt"
    " tag 300"
    " dcr p0"
    " jp 400"

    # command 3 (receive byte)
    " ld v3 p1"  # store Ack
    " call 3000"
    " halt"
    " tag 400"
    " dcr p0"
    " jp 500"

    # command 4 (transmit byte)
    " ld v4 p1"  # tx_byte
    " call 4000"
    " halt"
    " tag 500"
    " halt"

    # Start sequence.

    # v0 SCL, v1 SDA, v2 dly
    " tag 1000"
    " m v1 r"  # SDA=1 (pull-up)
    " mics v2"
    " m v0 r"  # SCL=1 (pull-up)
    " mics v2"
    " w v1 0 "  # SDA=0
    " mics v2"
    " w v0 0"  # SCL=0
    " mics v2"
    " ret"

    # Stop sequence.

    # v0 SCL, v1 SDA, v2 dly
    " tag 2000"
    " w v1 0"  # SDA=0
    " mics v2"
    " m v0 r"  # SCL=1 (pull-up)
    " mics v2"
    " m v1 r"  # SDA=1 (pull-up)
    " mics v2"
    " ret"

    # Receive byte.

    # v0 SCL, v1 SDA, v2 dly, v3 Ack
    " tag 3000"
    " ld v140 0"  # v140=0
    " m v1 r"  # SDA=1 (pull-up)
    " ld v141 8"  # v141=8
    " tag 3100"
    " rl v140 1"  # v140 = v140 * 2
    " m v0 r"  # SCL=1 (pull-up)
    " mics v2"
    " r v1"  # A=SDA
    " or v140"  # A|=v140
    " sta v140"  # v140=A
    " w v0 0"  # SCL=0
    " mics v2"
    " dcr v141"  # --v141
    " jnz 3100"  # loop x 8
    " lda v3"  # A=ack
    " dcra"  # ack 1?
    " jp 3200"
    " m v1 r"  # SDA=1 (pull-up)
    " jmp 3300"
    " tag 3200"
    " w v1 0"  # SDA=0
    " tag 3300"
    " mics v2"
    " m v0 r"  # SCL=1 (pull-up)
    " mics v2"
    " w v0 0"  # SCL=0
    " m v1 r"  # SDA=1 (pull-up)
    " lda v140"  # A=v140
    " sta p9"  # p9=A
    " ret"

    # Transmit byte.

    # v0 SCL, v1 SDA, v2 dly, v4 tx_byte
    " tag 4000"
    " ld v140 128"  # v140=128
    " ld v141 8"  # v141=8
    " tag 4100"
    " lda v4"  # A=tx_byte
    " and v140"  # A&=v140
    " jz 4200"
    " m v1 r"  # SDA=1 (pull-up)
    " jmp 4300"
    " tag 4200"
    " w v1 0"  # SDA=0
    " tag 4300"
    " mics v2"
    " m v0 r"  # SCL=1 (pull-up)
    " rr v140 1"  # v140 >> 1
    " mics v2"
    " w v0 0"  # SCL=0
    " dcr v141"  # --v141
    " jnz 4100"
    " m v1 r"  # SDA=1 (pull-up)
    " mics v2"
    " m v0 r"  # SCL=1 (pull-up)
    " mics v2"
    " r v1"  # get ack
    " sta p9"  # p9=ack
    " w v0 0"  # SCL=0
    " ret")


class I2C:
    def __init__(self, pi, SCL, SDA, baud):
        self.pi = pi
        self.SCL = SCL
        self.SDA = SDA

        self.p = [0] * 10
        self.baud = baud

        self.dly = 500000 / baud

        pi.set_mode(SCL, pigpio.OUTPUT)
        pi.set_mode(SDA, pigpio.OUTPUT)

        self.sid = pi.store_script(BB_I2C_SCRIPT)

        if self.sid >= 0:
            time.sleep(1.0)  # give time for script to start
            pi.run_script(self.sid, [0, SCL, SDA, self.dly])

    def wait_for_script(self, sid, delay):
        time.sleep(delay / 1000000.0)
        while True:
            s, self.p = self.pi.script_status(sid)
            if s != pigpio.PI_SCRIPT_RUNNING:
                break
            time.sleep(0.001)

    def S(self):
        self.pi.run_script(self.sid, [1])
        self.wait_for_script(self.sid, self.dly * 4)

    def E(self):
        self.pi.run_script(self.sid, [2])
        self.wait_for_script(self.sid, self.dly * 4)

    def RX(self, ack):
        self.pi.run_script(self.sid, [3, ack])
        self.wait_for_script(self.sid, self.dly * 8)
        return self.p[9]

    def TX(self, byte):
        self.pi.run_script(self.sid, [4, byte])
        self.wait_for_script(self.sid, self.dly * 8)
        return self.p[9]

    def cancel(self):
        """Cancel I2C"""
        self.pi.delete_script(self.sid)
