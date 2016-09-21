#ifndef _ESCAPE_SEQUENCE_
#define _ESCAPE_SEQUENCE_

#define JIS_0208_CP_COUNT 11104
extern const wchar_t cp_jis_0208[JIS_0208_CP_COUNT];

#define ISO_IR_165_CP_COUNT 8836
extern const wchar_t cp_iso_ir_165[ISO_IR_165_CP_COUNT];

#define KSC_5601_SYM_CP_COUNT 1115
extern const wchar_t cp_ksc5601_sym[KSC_5601_SYM_CP_COUNT];
#define KSC_5601_HANGUL_CP_COUNT 2350
extern const wchar_t cp_ksc5601_hangul[KSC_5601_HANGUL_CP_COUNT];
#define KSC_5601_HANJA_CP_COUNT 4888
extern const wchar_t cp_ksc5601_hanja[KSC_5601_HANJA_CP_COUNT];

#define PEND_1A 0x1a
#define ESC_1B  0x1b
#define SI_LS0  0xf
#define SO_LS1  0xe
#define ESC_LS2 0x6e
#define ESC_LS3 0x6f
#define ESC_SS2_7b  0x4e
#define ESC_SS2_8b  0x8e
#define ESC_SS3_7b  0x4f
#define ESC_SS3_8b  0x8f
#define ESC_LS1R    0x7e
#define ESC_LS2R    0x7d
#define ESC_LS3R    0x7c

enum iso_2022_charset { ascii, jp_roman, jp_katakana, jp_jis_1978, jp_jis_1983, cn_gb2312, cn_ir_165, ksc_5601 };

class escape_sequence
{
private:
    bool in_esc;
    char i1, i2, lead;
    char GL, GR, SS;
    iso_2022_charset G[4];
    void reset() { in_esc = false; i1 = '\0'; i2 = '\0'; lead = '\0'; };
    wchar_t translate(char c);
    static iso_2022_charset to_charset(unsigned short i1_f);

public:
    escape_sequence() : GL(0), GR(1), SS(0) { reset(); for(int i = 0; i < 4; ++i) G[i] = ascii; };
    escape_sequence& operator=(const escape_sequence& r) { in_esc = r.in_esc; i1 = r.i1; i2 = r.i2; lead = r.lead;
        GL = r.GL; GR = r.GR; SS = r.SS; for(int i = 0; i < 4; ++i) G[i] = r.G[i]; };
    escape_sequence(const escape_sequence& r) { *this = r; };
    wchar_t add_char(char c);
};

#endif
