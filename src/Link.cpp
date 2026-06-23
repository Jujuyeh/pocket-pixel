#include "Link.h"

#ifdef POCKET_PIXEL_FXC_LINK
#define I2C_IMPLEMENTATION
#define I2C_BUFFER_SIZE 12
#define I2C_CHECK_BUS_BUSY_CHECKS 255
#include <ArduboyI2C.h>

namespace {
constexpr uint8_t LINK_MAGIC = 0x50;
constexpr uint8_t LINK_VERSION = 1;
constexpr uint8_t LINK_KIND_BEACON = 1;
constexpr uint8_t LINK_KIND_INVITE = 2;
constexpr uint8_t LINK_KIND_INPUT = 3;
constexpr uint8_t LINK_KIND_STATE = 4;
constexpr uint8_t LINK_ADDRESS = 0x08;
constexpr uint8_t LINK_SEND_ADDRESS = 0x00;
constexpr uint8_t LINK_PEER_TIMEOUT_FRAMES = 45;
constexpr uint8_t LINK_SEND_INTERVAL_FRAMES = 8;
constexpr uint8_t LINK_TX_STUCK_FRAMES = 3;
constexpr uint8_t LINK_IDLE_LINE_CHECKS = 8;

struct LinkPacket {
    uint8_t magic;
    uint8_t version;
    uint8_t kind;
    uint8_t seq;
    uint8_t nonce;
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t f;
    uint8_t g;
};

volatile LinkPacket receivedPacket = {};
volatile bool hasReceivedPacket = false;

uint8_t localNonce = 1;
uint8_t peerNonce = 0;
uint8_t sequence = 0;
uint8_t sendFrame = 0;
uint8_t peerTimeout = 0;
uint8_t txActiveFrames = 0;
bool peerAvailable = false;
bool linkStarted = false;
bool pendingInvite = false;
bool pendingInput = false;
bool pendingState = false;
LinkInput latestInput = {};
LinkState latestState = {};

#if I2C_LIB_VER >= 30000
void onReceive(const uint8_t *buffer, uint8_t size);
#else
void onReceive();
#endif

void linkBeginBus() {
#if I2C_LIB_VER >= 30000
    I2C::begin();
#else
    I2C::init();
#endif
}

uint8_t linkError() {
#if I2C_LIB_VER >= 30000
    return I2C::error();
#else
    return I2C::getTWError();
#endif
}

void linkWritePacket(const LinkPacket &packet) {
#if I2C_LIB_VER >= 30000
    I2C::write(LINK_SEND_ADDRESS, packet, false);
#else
    I2C::write(LINK_SEND_ADDRESS, &packet, false);
#endif
}

bool linkLinesIdle() {
    for (uint8_t i = 0; i < LINK_IDLE_LINE_CHECKS; i++) {
        if (!(I2C_PIN & _BV(I2C_SDA_BIT)) || !(I2C_PIN & _BV(I2C_SCL_BIT))) {
            return false;
        }
        _delay_us(1000000.0 / I2C_FREQUENCY / 2.0);
    }
    return true;
}

void linkSetReceiveCallback() {
    I2C::onReceive(onReceive);
}

void linkResetBus() {
    i2c_detail::data.active = false;
    linkBeginBus();
    I2C::setAddress(LINK_ADDRESS, true);
    linkSetReceiveCallback();
    txActiveFrames = 0;
}

bool linkBusReady() {
    if (!linkStarted) {
        return false;
    }
    if (!i2c_detail::data.active) {
        txActiveFrames = 0;
        return true;
    }
    if (++txActiveFrames >= LINK_TX_STUCK_FRAMES) {
        linkResetBus();
    }
    return false;
}

void receivePacket(const uint8_t *buffer, uint8_t size) {
    if (size < sizeof(LinkPacket)) {
        return;
    }
    const LinkPacket *packet = reinterpret_cast<const LinkPacket *>(buffer);
    if (packet->magic != LINK_MAGIC || packet->version != LINK_VERSION) {
        return;
    }
    receivedPacket = *packet;
    hasReceivedPacket = true;
}

#if I2C_LIB_VER >= 30000
void onReceive(const uint8_t *buffer, uint8_t size) {
    receivePacket(buffer, size);
}
#else
void onReceive() {
    receivePacket(I2C::getBuffer(), sizeof(LinkPacket));
}
#endif

void sendPacket(LinkPacket packet) {
    if (!linkBusReady()) {
        return;
    }
    if (!linkLinesIdle()) {
        return;
    }
    packet.magic = LINK_MAGIC;
    packet.version = LINK_VERSION;
    packet.seq = sequence++;
    packet.nonce = localNonce;
    linkWritePacket(packet);
    if (linkError() != TW_SUCCESS) {
        i2c_detail::data.active = false;
    }
}

void processReceived() {
    if (!hasReceivedPacket) {
        return;
    }
    noInterrupts();
    LinkPacket packet = {};
    packet.magic = receivedPacket.magic;
    packet.version = receivedPacket.version;
    packet.kind = receivedPacket.kind;
    packet.seq = receivedPacket.seq;
    packet.nonce = receivedPacket.nonce;
    packet.a = receivedPacket.a;
    packet.b = receivedPacket.b;
    packet.c = receivedPacket.c;
    packet.d = receivedPacket.d;
    packet.e = receivedPacket.e;
    packet.f = receivedPacket.f;
    packet.g = receivedPacket.g;
    hasReceivedPacket = false;
    interrupts();

    if (packet.nonce == localNonce) {
        return;
    }
    peerNonce = packet.nonce;
    peerTimeout = LINK_PEER_TIMEOUT_FRAMES;
    peerAvailable = true;

    if (packet.kind == LINK_KIND_INVITE) {
        pendingInvite = true;
    } else if (packet.kind == LINK_KIND_INPUT) {
        latestInput.x = packet.a;
        latestInput.strike = packet.b != 0;
        pendingInput = true;
    } else if (packet.kind == LINK_KIND_STATE) {
        latestState.hostX = packet.a;
        latestState.clientX = packet.b;
        latestState.hostScore = packet.c;
        latestState.clientScore = packet.d;
        latestState.puckX = packet.e;
        latestState.puckY = packet.f;
        latestState.flags = packet.g;
        pendingState = true;
    }
}

void sendBeacon() {
    LinkPacket packet = {};
    packet.kind = LINK_KIND_BEACON;
    sendPacket(packet);
}
}

void linkBegin(uint32_t seed) {
    localNonce = static_cast<uint8_t>((seed ^ (seed >> 8) ^ (seed >> 16)) & 0x7F);
    if (localNonce == 0) {
        localNonce = 1;
    }
    sendFrame = localNonce % LINK_SEND_INTERVAL_FRAMES;
    linkStarted = true;
    linkResetBus();
}

void linkUpdate(bool advertise) {
    linkBusReady();
    processReceived();
    if (peerTimeout > 0) {
        peerTimeout--;
    } else {
        peerAvailable = false;
    }
    if (advertise && sendFrame++ >= LINK_SEND_INTERVAL_FRAMES) {
        sendFrame = 0;
        sendBeacon();
    }
}

bool linkPeerAvailable() {
    return peerAvailable;
}

bool linkLocalIsHost() {
    return localNonce >= peerNonce;
}

bool linkConsumeInvite() {
    if (!pendingInvite) {
        return false;
    }
    pendingInvite = false;
    return true;
}

void linkSendInvite() {
    LinkPacket packet = {};
    packet.kind = LINK_KIND_INVITE;
    sendPacket(packet);
}

void linkSendInput(uint8_t x, bool strike) {
    LinkPacket packet = {};
    packet.kind = LINK_KIND_INPUT;
    packet.a = x;
    packet.b = strike ? 1 : 0;
    sendPacket(packet);
}

bool linkConsumeInput(LinkInput &input) {
    if (!pendingInput) {
        return false;
    }
    input = latestInput;
    pendingInput = false;
    return true;
}

void linkSendState(const LinkState &state) {
    LinkPacket packet = {};
    packet.kind = LINK_KIND_STATE;
    packet.a = state.hostX;
    packet.b = state.clientX;
    packet.c = state.hostScore;
    packet.d = state.clientScore;
    packet.e = state.puckX;
    packet.f = state.puckY;
    packet.g = state.flags;
    sendPacket(packet);
}

bool linkConsumeState(LinkState &state) {
    if (!pendingState) {
        return false;
    }
    state = latestState;
    pendingState = false;
    return true;
}

#else

void linkBegin(uint32_t) {}
void linkUpdate(bool) {}
bool linkPeerAvailable() { return false; }
bool linkLocalIsHost() { return true; }
bool linkConsumeInvite() { return false; }
void linkSendInvite() {}
void linkSendInput(uint8_t, bool) {}
bool linkConsumeInput(LinkInput &) { return false; }
void linkSendState(const LinkState &) {}
bool linkConsumeState(LinkState &) { return false; }

#endif
