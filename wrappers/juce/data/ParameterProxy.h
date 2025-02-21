//
// Created by Paul Walker on 1/15/22.
//

#ifndef SHORTCIRCUIT_PARAMETERPROXY_H
#define SHORTCIRCUIT_PARAMETERPROXY_H

#include <fmt/core.h>

#include "configuration.h"
#include "sampler_wrapper_actiondata.h"
#include "wrapper_msg_to_string.h"
#include "data/BasicTypes.h"
#include <cmath>

namespace scxt
{
namespace data
{

template <typename T> struct SendVGA
{
};

template <> struct SendVGA<float>
{
    static constexpr VAction action = vga_floatval;
};
template <> struct SendVGA<int>
{
    static constexpr VAction action = vga_intval;
};
template <> struct SendVGA<std::string>
{
    static constexpr VAction action = vga_text;
};

template <typename T, VAction A = SendVGA<T>::action> struct ParameterProxy
{
    ParameterProxy() {}
    int id{-1};
    int subid{-1};
    T val{0};
    bool hidden{false};
    bool disabled{false};
    bool temposync{false};
    std::string label;

    T min{0}, max{1}, def{0}, step;
    std::string units;
    bool paramRangesSet{false};
    bool complainedAboutParamRangesSet{false};

    enum Units
    {
        DEF,
        DB,
        HERTZ,
        KHERTZ,
        LOWFREQHERTZ,
        PERCENT,
        SECONDS,
        SEMITONES,
        CENTS,
        OCTAVE,
        MIDIKEY
    } unitType{DEF};

    void parseDatamode(const char *datamode)
    {
        // syntax is char,min,step,max,def,units
        std::string dm{datamode};
        std::vector<std::string> elements;
        while (true)
        {
            auto p = dm.find(',');
            if (p == std::string::npos)
            {
                elements.push_back(dm);
                break;
            }
            else
            {
                auto r = dm.substr(0, p);
                dm = dm.substr(p + 1);
                elements.push_back(r);
            }
        }
        jassert(elements[0][0] == dataModeIndicator());

        if (elements[0][0] == 'i')
        {
            if (elements.size() != 5)
                DBG("BAD E [" << datamode << "] " << getName());
            jassert(elements.size() == 5);
            min = strToT(elements[1]);
            max = strToT(elements[2]);
            def = strToT(elements[3]);
            step = 1;
            units = elements[4];
        }
        else
        {
            jassert(elements.size() == 6);
            min = strToT(elements[1]);
            step = strToT(elements[2]);
            max = strToT(elements[3]);
            def = strToT(elements[4]);
            units = elements[5];
        }
        paramRangesSet = true;

        unitType = DEF;
        if (units == "dB")
            unitType = DB;
        if (units == "Hz")
            unitType = HERTZ;
        if (units == "LFHz")
            unitType = LOWFREQHERTZ;
        if (units == "%")
            unitType = PERCENT;
        if (units == "s")
            unitType = SECONDS;
        if (units == "kHz")
            unitType = KHERTZ;
        if (units == "oct")
            unitType = OCTAVE;
        if (units == "st")
            unitType = SEMITONES;
        if (units == "cents")
            unitType = CENTS;
        if (units == "midikey")
            unitType = MIDIKEY;

        if (unitType == DEF)
        {
            // std::cout << "UNIT [" << units << "]" << " dm=[" << datamode << "]" << std::endl;
            // jassert(unitType != DEF);
        }
    }

    std::string valueToStringWithUnits() const
    {
        switch (unitType)
        {
        case DB:
            return fmt::format("{:.2f} dB", val);
        case PERCENT:
            return fmt::format("{:.2f} %", val * 100);
        case OCTAVE:
            return fmt::format("{:.2f} oct", val);
        case SECONDS:
        {
            auto res = fmt::format("{:.3f} s", pow(2.0, val));
            return res;
        }
        case HERTZ:
        {
            auto res = fmt::format("{:.3f} Hz", 440 * pow(2.0, val));
            return res;
        }
        case LOWFREQHERTZ:
        {
            if (temposync)
            {
                auto res = fmt::format("{}", temposynced_float_to_str(val));
                return res;
            }
            auto res = fmt::format("{:.3f} Hz", pow(2.0, val));
            return res;
        }
        case KHERTZ:
        {
            auto res = fmt::format("{:.3f} kHz", 440 * pow(2.0, val) / 1000.0);
            return res;
        }

        case CENTS:
        {
            auto res = fmt::format("{:.3f} ct", val);
            return res;
        }

        case SEMITONES:
        {
            auto res = fmt::format("{:.3f} semi", val);
            return res;
        }

        case DEF:
        default:
            return fmt::format("{:.3f} RAW ({})", val, units);
        }
    }

    bool isBipolar()
    {
        if (unitType == DEF || unitType == PERCENT || unitType == DB || unitType == CENTS ||
            unitType == OCTAVE)
            return min * max < 0;
        return false;
    }

    // Float vs Int a bit clumsy here
    float getValue01() const { return std::clamp((val - min) / (max - min), 0.f, 1.f); }
    void sendValue01(float value01, ActionSender *s) { jassert(false); }
    void sendValue(const T &value, ActionSender *s) { jassertfalse; }

    std::string value_to_string() const { return std::to_string(val); }

    char dataModeIndicator() const { return 'x'; }
    T strToT(const std::string &s)
    {
        jassertfalse;
        return T{};
    }

    std::string getLabel() const
    {
        if (label.empty())
            return getName();
        return label;
    }

    std::string getName() const
    {
        if (id > 0)
            return ip_data[id].name;
        return "";
    }

    template <typename Q> void setFrom(const Q &v) { jassertfalse; }

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void onChanged() = 0;
    };
    std::set<Listener *> listeners;
    void addListener(Listener *l) { listeners.insert(l); }
    void removeListener(Listener *l) { listeners.erase(l); }
    void notifyListeners() const
    {
        for (auto *l : listeners)
            l->onChanged();
    }

    float temposync_quantitize(float x) const
    {
        float a{0};
        float b = std::modf(x, &a);
        if (b < 0)
        {
            b += 1.f;
            a -= 1.f;
        }
        b = pow(2.0, b);
        b = std::min(floor(b * 2.f) / 2.f, floor(b * 3.f) / 3.f);
        b = std::log(b) / std::log(2.f);
        return a + b;
    }

    std::string temposynced_float_to_str(float x) const
    {
        // Obviously consider replacing this with the improved version we have in Surge
        // one day soon
        x = temposync_quantitize(x);
        char txt[256];

        if (x > 4)
            sprintf(txt, "%.2f / 64th", 32.f * powf(2.0f, -x));
        else if (x > 3)
            sprintf(txt, "%.2f / 32th", 16.f * powf(2.0f, -x));
        else if (x > 2)
            sprintf(txt, "%.2f / 16th", 8.f * powf(2.0f, -x));
        else if (x > 1)
            sprintf(txt, "%.2f / 8th", 4.f * powf(2.0f, -x));
        else if (x > 0)
            sprintf(txt, "%.2f / 4th", 2.f * powf(2.0f, -x));
        else if (x > -1)
            sprintf(txt, "%.2f / 2th", 1.f * powf(2.0f, -x));
        else
            sprintf(txt, "%.2f bars", 0.5f * powf(2.0f, -x));

        return txt;
    }
};

template <> inline std::string ParameterProxy<int>::value_to_string() const
{
    if (unitType == MIDIKEY)
    {
        static auto keys =
            std::array{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int kn = val % 12;
        int oc = val / 12;
        return fmt::format("{}{}", keys[kn], oc);
    }
    else
        return fmt::format("{}", val);
}

template <> template <> inline void ParameterProxy<float>::setFrom<float>(const float &v)
{
    if (paramRangesSet)
    {
        val = std::clamp(v, min, max);
        if (v < min || v > max)
        {
            DBG("Clamping range on control " << debug_wrapper_ip_to_string(id)
                << " with min-max = " << min << "/" << max << " and val=" << v );
        }
    }
    else
    {
        val = v;
    }
    notifyListeners();
}
template <> template <> inline void ParameterProxy<int>::setFrom<int>(const int &v)
{
    val = v;
    notifyListeners();
}

template <>
template <>
inline void ParameterProxy<std::string>::setFrom<std::string>(const std::string &v)
{
    val = v;
    notifyListeners();
}

template <> inline std::string ParameterProxy<float>::value_to_string() const
{
    return fmt::format("{:.2f}", val);
}
template <> inline char ParameterProxy<float>::dataModeIndicator() const { return 'f'; }
template <> inline float ParameterProxy<float>::strToT(const std::string &s)
{
    // Ideally we would use  auto [ptr, ec]{std::from_chars(s.data(), s.data() + s.size(), val,
    // std::chars_format::general)}; but the runtimes don't support it yet
    float res = 0.0f;
    std::istringstream istr(s);

    istr.imbue(std::locale::classic());
    istr >> res;
    return res;
}

template <>
inline void ParameterProxy<float, vga_floatval>::sendValue01(float value01, ActionSender *s)
{
    auto v = value01 * (max - min) + min;
    val = v;
    actiondata ad;
    ad.id = id;
    ad.subid = subid;
    ad.actiontype = vga_floatval;
    ad.data.f[0] = v;
    s->sendActionToEngine(ad);
    notifyListeners();
}
template <>
inline void ParameterProxy<float, vga_floatval>::sendValue(const float &value, ActionSender *s)
{
    val = value;
    actiondata ad;
    ad.id = id;
    ad.subid = subid;
    ad.actiontype = vga_floatval;
    ad.data.f[0] = val;
    if (temposync)
        ad.data.f[0] = temposync_quantitize(val);
    s->sendActionToEngine(ad);
    notifyListeners();
}

template <>
inline void ParameterProxy<float, vga_steplfo_data_single>::sendValue(const float &value,
                                                                      ActionSender *s)
{
    // This shuld never be called; because of the message layout the message is generated
    // specially in the ZonePage
    std::terminate();
}

template <> inline char ParameterProxy<int>::dataModeIndicator() const { return 'i'; }
template <> inline int ParameterProxy<int>::strToT(const std::string &s)
{
    return std::atoi(s.c_str());
}
template <> inline float ParameterProxy<int>::getValue01() const
{
    jassert(false);
    return 0;
}
template <> inline void ParameterProxy<int>::sendValue(const int &value, ActionSender *s)
{
    actiondata ad;
    ad.id = id;
    ad.subid = subid;
    ad.actiontype = vga_intval;
    ad.data.i[0] = value;
    val = value;
    s->sendActionToEngine(ad);
    notifyListeners();
}

template <>
inline void ParameterProxy<std::string, vga_text>::sendValue(const std::string &value,
                                                             ActionSender *s)
{
    val = value;
    actiondata ad;
    ad.id = id;
    ad.subid = subid;
    ad.actiontype = vga_text;
    strncpy(ad.data.str, val.c_str(), 54);
    s->sendActionToEngine(ad);
    notifyListeners();
}
} // namespace data
} // namespace scxt

#endif // SHORTCIRCUIT_PARAMETERPROXY_H
