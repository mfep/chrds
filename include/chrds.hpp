#ifndef CHRDS_HPP_
#define CHRDS_HPP_

#include <array>
#include <concepts>
#include <iterator>
#include <ranges>
#include <span>
#include <stdexcept>

#include <cassert>
#include <cstddef>
#include <cstdint>

#define CHRDS_ASSERT(expression) assert(expression)

namespace chrds::midi
{

enum class MessageKind
{
    NoteOff,
    NoteOn,
    PolyAftertouch,
    ControlChange,
    ProgramChange,
    ChannelAftertouch,
    PitchWheel,
    SystemExclusive
};

struct InvalidMidiData : public std::runtime_error
{
    InvalidMidiData() : std::runtime_error("Invalid MIDI message data encountered") {}
};

template <class T>
concept ByteSized = (sizeof(T) == 1) && requires(T val) {
    {
        static_cast<std::uint8_t>(val)
    };
    {
        static_cast<std::uint16_t>(val)
    };
};

template <ByteSized T>
class MessageView
{
public:
    constexpr explicit MessageView(std::span<T, 3> data) noexcept : _data(data) {}

    constexpr T status() const noexcept { return _data[0]; }

    constexpr T data_0() const noexcept { return _data[1]; }

    constexpr T data_1() const noexcept { return _data[2]; }

    constexpr MessageKind kind() const
    {
        switch (static_cast<std::uint8_t>(status()) & 0xf0)
        {
        // The first bit is always 1 according to the spec
        case 0x80:
            return MessageKind::NoteOff;
        case 0x90:
            return MessageKind::NoteOn;
        case 0xA0:
            return MessageKind::PolyAftertouch;
        case 0xB0:
            return MessageKind::ControlChange;
        case 0xC0:
            return MessageKind::ProgramChange;
        case 0xD0:
            return MessageKind::ChannelAftertouch;
        case 0xE0:
            return MessageKind::PitchWheel;
        case 0xF0:
            return MessageKind::SystemExclusive;
        default:
            throw InvalidMidiData();
        }
    }

    constexpr std::uint8_t channel() const
    {
        CHRDS_ASSERT(kind() != MessageKind::SystemExclusive);
        return static_cast<std::uint8_t>(status()) & 0x0f;
    }

    constexpr std::uint8_t note() const
    {
        constexpr std::array expected_kinds{
            MessageKind::NoteOff,
            MessageKind::NoteOn,
            MessageKind::PolyAftertouch,
        };
        CHRDS_ASSERT(std::ranges::find(expected_kinds, kind()) != expected_kinds.end());
        return static_cast<std::uint8_t>(data_0());
    }

    constexpr std::uint8_t velocity() const
    {
        constexpr std::array expected_kinds{
            MessageKind::NoteOff,
            MessageKind::NoteOn,
        };
        CHRDS_ASSERT(std::ranges::find(expected_kinds, kind()) != expected_kinds.end());
        return static_cast<std::uint8_t>(data_1());
    }

    constexpr std::uint8_t pressure() const
    {
        switch (kind())
        {
        case MessageKind::PolyAftertouch:
            return static_cast<std::uint8_t>(data_0());
        case MessageKind::ChannelAftertouch:
            return static_cast<std::uint8_t>(data_1());
        default:
            CHRDS_ASSERT(false);
            return 0;
        }
    }

    constexpr std::uint8_t cc_controller() const
    {
        CHRDS_ASSERT(kind() == MessageKind::ControlChange);
        return static_cast<uint8_t>(data_0());
    }

    constexpr std::uint8_t cc_value() const
    {
        CHRDS_ASSERT(kind() == MessageKind::ControlChange);
        return static_cast<uint8_t>(data_1());
    }

    constexpr std::uint8_t program_number() const
    {
        CHRDS_ASSERT(kind() == MessageKind::ProgramChange);
        return static_cast<uint8_t>(data_0());
    }

    constexpr std::int16_t pitch_wheel() const
    {
        return static_cast<std::int16_t>((static_cast<std::uint16_t>(data_1()) << 8) +
                                         static_cast<std::uint16_t>(data_0()));
    }

private:
    std::span<T, 3> _data;
};

struct InvalidMidiDataLength : public std::runtime_error
{
    InvalidMidiDataLength()
        : std::runtime_error("The length of the MIDI data is not divisible by 3")
    {
    }
};

template <ByteSized T>
class MessagesView
{
public:
    constexpr explicit MessagesView(std::span<T> data) : _data(data)
    {
        if (data.size() % 3 != 0)
        {
            throw InvalidMidiDataLength();
        }
    }

private:
    using span_iter = typename std::span<T>::iterator;
    struct MessageIterator
    {
        using difference_type = std::ptrdiff_t;
        using value_type      = MessageView<T>;

        constexpr MessageIterator& operator++() noexcept
        {
            _span_it += 3;
            return *this;
        }
        constexpr MessageIterator operator++(int) noexcept
        {
            auto copy = *this;
            this->operator++();
            return copy;
        }
        constexpr MessageIterator& operator--() noexcept
        {
            _span_it -= 3;
            return *this;
        }
        constexpr MessageIterator operator--(int) noexcept
        {
            auto copy = *this;
            this->operator--();
            return copy;
        }
        constexpr difference_type operator-(const MessageIterator& other) const noexcept
        {
            return (_span_it - other._span_it) / 3;
        }
        constexpr MessageIterator operator-(const difference_type delta) const noexcept
        {
            return MessageIterator{_span_it - 3 * delta};
        }
        friend constexpr MessageIterator operator-(const difference_type  delta,
                                                   const MessageIterator& iter)
        {
            return iter - delta;
        }
        constexpr MessageIterator& operator-=(const difference_type delta) noexcept
        {
            _span_it -= delta * 3;
            return *this;
        }
        constexpr MessageIterator operator+(const difference_type delta) const noexcept
        {
            return MessageIterator{_span_it + 3 * delta};
        }
        friend constexpr MessageIterator operator+(const difference_type  delta,
                                                   const MessageIterator& iter)
        {
            return iter + delta;
        }
        constexpr MessageIterator& operator+=(const difference_type delta) noexcept
        {
            _span_it += delta * 3;
            return *this;
        }
        constexpr value_type operator*() const noexcept
        {
            return value_type(std::span<T, 3>(_span_it, 3));
        }
        constexpr value_type operator[](difference_type delta) const noexcept
        {
            return *(*this + delta);
        }
        constexpr friend auto operator<=>(const MessageIterator&, const MessageIterator&) = default;

        span_iter _span_it;
    };
    static_assert(std::random_access_iterator<MessageIterator>);

    std::span<T> _data;
};

} // namespace chrds::midi

#endif // CHRDS_HPP_
