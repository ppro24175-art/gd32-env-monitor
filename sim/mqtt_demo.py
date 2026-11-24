"""Pass an application payload through a real localhost MQTT broker."""

import argparse
import asyncio
import json
from pathlib import Path
from queue import Queue
import subprocess
import sys
from threading import Event


ROOT = Path(__file__).resolve().parents[1]
TOPIC = "env-monitor/telemetry"
LOCAL_PACKAGES = ROOT / ".tools" / "mqtt"
if LOCAL_PACKAGES.is_dir():
    sys.path.insert(0, str(LOCAL_PACKAGES))

from amqtt.broker import Broker
from paho.mqtt import client, publish


def subscribe_and_publish(payload, port):
    subscribed = Event()
    received = Queue()
    subscriber = client.Client(client.CallbackAPIVersion.VERSION2, client_id="env-demo-sub")
    subscriber.on_connect = lambda connection, *_: connection.subscribe(TOPIC)
    subscriber.on_subscribe = lambda *_: subscribed.set()
    subscriber.on_message = lambda _, __, message: received.put((message.topic, message.payload))
    subscriber.connect("127.0.0.1", port)
    subscriber.loop_start()
    try:
        if not subscribed.wait(5):
            raise RuntimeError("No SUBACK from the local broker")
        print(f"[broker] MQTT subscriber listening on {TOPIC}", flush=True)
        publish.single(TOPIC, payload, hostname="127.0.0.1", port=port, client_id="env-demo-pub")
        topic, data = received.get(timeout=5)
        if topic != TOPIC or data != payload.encode():
            raise RuntimeError("MQTT subscriber received a different topic or payload")
        print(f"[subscriber] {topic} {data.decode()}", flush=True)
    finally:
        subscriber.disconnect()
        subscriber.loop_stop()


async def demo(payload, port):
    broker = Broker({
        "listeners": {"default": {"type": "tcp", "bind": f"127.0.0.1:{port}"}},
        "plugins": {"amqtt.plugins.authentication.AnonymousAuthPlugin": {"allow_anonymous": True}},
    })
    await broker.start()
    try:
        await asyncio.wait_for(asyncio.to_thread(subscribe_and_publish, payload, port), 20)
    finally:
        await broker.shutdown()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scenario", default="normal",
                        choices=("normal", "checksum", "dht-timeout", "light-missing", "reconnect"))
    parser.add_argument("--port", default=18883, type=int)
    args = parser.parse_args()
    output = subprocess.check_output(
        [sys.executable, str(ROOT / "build.py"), "simulate", args.scenario],
        cwd=ROOT, encoding="utf-8",
    )
    print(output, end="", flush=True)
    marker = "PUBLISH "
    payloads = [line[len(marker):] for line in output.splitlines() if line.startswith(marker)]
    if len(payloads) != 1:
        raise SystemExit(f"Expected one successful publication, got {len(payloads)}")
    payload = payloads[0]
    json.loads(payload)
    asyncio.run(demo(payload, args.port))


if __name__ == "__main__":
    main()
