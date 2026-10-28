// Built-in AMBE+2 vocoder, see softwarevocoder.h.
//
// The encoder yields the nine AMBE+2 parameters b0..b8; the codec does not
// send them one after another but spreads their low bits over the end of the
// 49-bit frame. kBitPositions is that layout, as mbelib reads it back.

#include "softwarevocoder.h"

#include <QtEndian>

extern "C" {
#include "mbelib.h"
}
#include "mbeenc.h"

namespace {

constexpr int kParameters = 9;
constexpr int kParameterBits[kParameters] = {7, 5, 5, 9, 7, 5, 4, 4, 3};
// Frame bit of each parameter bit, most significant first.
constexpr int kBitPositions[kParameters][9] = {
    {0, 1, 2, 3, 37, 38, 39},
    {4, 5, 6, 7, 35},
    {8, 9, 10, 11, 36},
    {12, 13, 14, 15, 16, 17, 18, 19, 40},
    {20, 21, 22, 23, 41, 42, 43},
    {24, 25, 26, 27, 44},
    {28, 29, 30, 45},
    {31, 32, 33, 46},
    {34, 47, 48},
};

// The encoder subtracts this from the log2 frame gain. 4.5 makes speech come
// out of the encoder + mbelib chain at the level it went in (measured with
// synthetic vowels); lower values overdrive the stations listening.
constexpr float kEncoderGainLog2 = 4.5f;
// mbelib's synthesis quality setting for unvoiced bands (its usual value).
constexpr int kUnvoicedQuality = 3;

} // namespace

struct SoftwareVocoder::Codec {
    mbe_parms current;
    mbe_parms previous;
    mbe_parms previousEnhanced;
    std::unique_ptr<MBEEncoder> encoder;

    Codec() { reset(); }

    void reset()
    {
        mbe_initMbeParms(&current, &previous, &previousEnhanced);
        encoder.reset(new MBEEncoder);
        encoder->set_dmr_mode();
        encoder->set_gain_adjust(kEncoderGainLog2);
    }

    QByteArray decode(const AmbeFrame::Frame49 &frame);
    AmbeFrame::Frame49 encode(const QByteArray &pcm);
};

QByteArray SoftwareVocoder::Codec::decode(const AmbeFrame::Frame49 &frame)
{
    char bits[AmbeFrame::kFrame49Bits];
    for (int i = 0; i < AmbeFrame::kFrame49Bits; ++i)
        bits[i] = char((frame[i / 8] >> (7 - i % 8)) & 1);

    short samples[kSamplesPerFrame];
    int errors = 0;
    int errors2 = 0;
    char errorText[64] = {};
    mbe_processAmbe2450Data(samples, &errors, &errors2, errorText, bits, &current, &previous,
                            &previousEnhanced, kUnvoicedQuality);

    QByteArray pcm(kPcmBytesPerFrame, '\0');
    auto *out = reinterpret_cast<uchar *>(pcm.data());
    for (int i = 0; i < kSamplesPerFrame; ++i)
        qToLittleEndian(qint16(samples[i]), out + 2 * i);
    return pcm;
}

AmbeFrame::Frame49 SoftwareVocoder::Codec::encode(const QByteArray &pcm)
{
    int16_t samples[kSamplesPerFrame];
    const auto *in = reinterpret_cast<const uchar *>(pcm.constData());
    for (int i = 0; i < kSamplesPerFrame; ++i)
        samples[i] = qFromLittleEndian<qint16>(in + 2 * i);

    int parameters[kParameters] = {};
    encoder->encode_dmr_params(samples, parameters);

    AmbeFrame::Frame49 frame{};
    for (int p = 0; p < kParameters; ++p) {
        for (int j = 0; j < kParameterBits[p]; ++j) {
            const int bit = kBitPositions[p][j];
            if ((parameters[p] >> (kParameterBits[p] - 1 - j)) & 1)
                frame[bit / 8] |= uint8_t(0x80 >> (bit % 8));
        }
    }
    return frame;
}

SoftwareVocoder::SoftwareVocoder(QObject *parent)
    : Vocoder(parent), m_codec(new Codec), m_worker(new QObject)
{
    m_worker->moveToThread(&m_thread);
    m_thread.setObjectName(QStringLiteral("vocoder"));
    m_thread.start();
}

SoftwareVocoder::~SoftwareVocoder()
{
    m_thread.quit();
    m_thread.wait();
    delete m_worker;
}

// Each call is queued to the worker thread in order; each result is queued
// back to this object's thread.
void SoftwareVocoder::reset()
{
    QMetaObject::invokeMethod(m_worker, [this] { m_codec->reset(); }, Qt::QueuedConnection);
}

void SoftwareVocoder::decode(const AmbeFrame::Frame49 &frame)
{
    QMetaObject::invokeMethod(m_worker, [this, frame] {
        const QByteArray pcm = m_codec->decode(frame);
        QMetaObject::invokeMethod(this, [this, pcm] { emit decoded(pcm); },
                                  Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void SoftwareVocoder::encode(const QByteArray &pcm)
{
    if (pcm.size() < kPcmBytesPerFrame)
        return;
    QMetaObject::invokeMethod(m_worker, [this, pcm] {
        const AmbeFrame::Frame49 frame = m_codec->encode(pcm);
        QMetaObject::invokeMethod(this, [this, frame] { emit encoded(frame); },
                                  Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}
