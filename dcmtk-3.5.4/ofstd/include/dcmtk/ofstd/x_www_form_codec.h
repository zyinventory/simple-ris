#ifndef X_WWW_FORM_CODEC_H
#define X_WWW_FORM_CODEC_H

#include <cstdio>

/** http://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.1
 *  Control names and values are escaped. Space characters are replaced by `+', 
 *  and then reserved characters are escaped as described in [RFC1738], section 2.2: 
 *  Non-alphanumeric characters are replaced by `%HH', 
 *  a percent sign and two hexadecimal digits representing the ASCII code of the character. 
 *  Line breaks are represented as "CR LF" pairs (i.e., `%0D%0A').
 */
namespace x_www_form_codec_namespace
{
    template<typename T> void impl_output_char(T **ot, char c) { (**ot) << c; }
    template<> void impl_output_char<char>(char **ot, char c) { *(*ot)++ = c; }
    template<> void impl_output_char<FILE>(FILE **ot, char c) { fputc(c, *ot); }
    template<> void impl_output_char<std::string>(std::string **ot, char c) { (*ot)->append(1, c); }
}

template<typename T>
class x_www_form_codec
{
private:
    static char enc_char(char in)
    {
        switch (in) {
        // alpha num $-_.!*'(), and \0(indicate end of string)
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case '$': case '-': case '_': case '.': case '!':
        case '*': case '\'': case '(': case ')': case ',': case '\0':
            return in;
        case ' ':
            return '+';
        default:
            return '%';
        }
    }

    static char ToHex(char x) { return x > 9 ? x + 55 : x + 48; }

    static char FromHex(char x)
    {
        char y;
        if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
        else if (x >= '0' && x <= '9') y = x - '0';
        else y = '?';
        return y;
    }
public:
    static int encode(const char *in, T *toAppend, int out_limit = INT_MAX, int in_len = INT_MAX);
    static int decode(const char *in, T *toAppend, int out_limit = INT_MAX, int in_len = INT_MAX);
};

template<typename T>
int x_www_form_codec<T>::encode(const char *in, T *toAppend, int out_limit, int in_len)
{
    int cnt = 0;
    bool notTerm = true;
    for(int i = 0; i < in_len && notTerm && cnt < out_limit; ++i)
    {
        char e = enc_char(in[i]);
        switch(e)
        {
        case '\0':
            notTerm = false;
            break;
        case '%':
            if(cnt + 3 <= out_limit)
            {
                x_www_form_codec_namespace::impl_output_char(&toAppend, e);
                x_www_form_codec_namespace::impl_output_char(&toAppend, ToHex(in[i] >> 4));
                x_www_form_codec_namespace::impl_output_char(&toAppend, ToHex(in[i] % 16));
                cnt += 3;
            }
            else notTerm = false;
            break;
        default:
            x_www_form_codec_namespace::impl_output_char(&toAppend, e);
            ++cnt;
            break;
        }
    }
    return cnt;
}

template<typename T>
int x_www_form_codec<T>::decode(const char *in, T *toAppend, int out_limit, int in_len)
{
    int cnt = 0;
    bool notTerm = true;
    for (int i = 0; i < in_len && notTerm && cnt < out_limit; ++i)
    {
        switch(in[i])
        {
        case '\0':
            notTerm = false;
            break;
        case '+':
            x_www_form_codec_namespace::impl_output_char(&toAppend, ' ');
            ++cnt;
            break;
        case '%':
            if(i + 3 <= in_len)
            {
                char h = FromHex(in[++i]);
                char l = FromHex(in[++i]);
                if(h > 0xF || l > 0xF)
                    x_www_form_codec_namespace::impl_output_char(&toAppend, '?');
                else
                    x_www_form_codec_namespace::impl_output_char(&toAppend, (h << 4) + l);
                ++cnt;
            }
            else notTerm = false;
            break;
        default:
            x_www_form_codec_namespace::impl_output_char(&toAppend, in[i]);
            ++cnt;
        }
    }
    return cnt;
}

#endif
