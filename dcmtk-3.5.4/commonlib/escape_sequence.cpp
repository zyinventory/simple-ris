#include "stdafx.h"
#include "escape_sequence.h"

using namespace std;

wchar_t escape_sequence::translate(char ic)
{
    char c = (ic & 0x7f);

    iso_2022_charset cs = ascii;
    if(SS) cs = G[SS];
    else
    {
        if((lead ? lead : ic) & 0x80) cs = G[GR];
        else cs = G[GL];
    }

    switch(cs)
    {
    case jp_katakana:
        SS = 0;
        lead = '\0';    //SBCS
        if(c > 0x20 && c < 0x60) return 0xFF61 - 0x21 + c;
        else return 0;
    case ascii:
        SS = 0;
        lead = '\0';    //SBCS
        return c;
    case jp_roman:
        SS = 0;
        lead = '\0';    //SBCS
        if     (c == 0x5C) return 0xA5;
        else if(c == 0x7E) return 0x203E;
        else return c;
    case jp_jis_0212:
        if(lead)    //MBCS
        {   // is tail
            wchar_t wch = 0;
            if(c > 0x20 && c < 0x7f)
            {
                int index = ((lead & 0x7f) - 0x21) * 94 + c - 0x21;
                int i = 0;
                while(index > jisx0212_to_ucs_idx[i][1]) ++i;
                if(index >= jisx0212_to_ucs_idx[i][0])
                {
                    int cp = jisx0212_to_ucs_idx[i][2] - jisx0212_to_ucs_idx[i][0] + index;
                    if(cp < JIS_0212_CP_COUNT && cp >= 0) wch = cp_jis_0212[cp];
                }
            }
            lead = '\0';
            SS = 0;
            return wch;
        }
        else
        {   // is lead
            if(c < 0x22 || c > 0x6d)
            {
                lead = '\0';
                SS = 0;
                return 0;
            }
            else
            {
                lead = ic;
                return PEND_1A;
            }
        }
    case cn_gb2312:
    case cn_ir_165:
    case jp_jis_1978:
    case jp_jis_0208:
    case ksc_5601:
        if(c < 0x21 || c > 0x7e)
        {
            lead = '\0';
            SS = 0;
            return 0;
        }
        if(lead)    //MBCS
        {   // is tail
            int index = ((lead & 0x7f) - 0x21) * 94 + c - 0x21;
            lead = '\0';
            SS = 0;
            int bounder = 0;
            const wchar_t *dict = NULL;

            if(cs == jp_jis_0208)
            {
                bounder = JIS_0208_CP_COUNT;
                dict = cp_jis_0208;
            }
            else if(cs == cn_gb2312 || cs == cn_ir_165)
            {
                bounder = ISO_IR_165_CP_COUNT;
                dict = cp_iso_ir_165;
            }
            else if(cs == ksc_5601)
            {   /* 1410 = 15 * 94 , 3760 = 40 * 94
                Hangul in KS C 5601 : row 16 - row 40 */
                if(index - 3854 >= 0 && index - 3854 < KSC_5601_HANJA_CP_COUNT) /* Hanja : row 42 - row 93 : 3854 = 94 * (42-1) */
                {   // index >= 3854
                    index -= 3854;
                    dict = cp_ksc5601_hanja;
                }
                else if (index - 1410 >= 0 && index - 1410 < KSC_5601_HANGUL_CP_COUNT)
                {   // index >= 1410 && index < 1410 + KSC_5601_HANGUL_CP_COUNT
                    index -= 1410;
                    dict = cp_ksc5601_hangul;
                }
                else if(index - 1114 >= 0 && index - 1114 < KSC_5601_SYM_CP_COUNT)
                {   // index <= 1114
                    index -= 1114;
                    dict = cp_ksc5601_sym;
                }
            }

            if(dict && index >= 0 && index < bounder) return dict[index];
            else return 0;
        }
        else
        {   // is lead
            lead = ic;
            return PEND_1A;
        }
    default:
        break; //fallback
    }
    lead = '\0';
    SS = 0;
    return 0;
}

iso_2022_charset escape_sequence::to_charset(unsigned short i1_f)
{
    switch(i1_f)
    {
    case 0x42:
        return ascii;
    case 0x43:
        return ksc_5601;
    case 0x44:
        return jp_jis_0212;
    case 0x45:
        return cn_ir_165;
    case 0x49:
        return jp_katakana;
    case 0x4A:
        return jp_roman;
    case 0x2440:
        return jp_jis_1978;
    case 0x41:
    case 0x2441:
        return cn_gb2312;
    case 0x2442:
        return jp_jis_0208;
    default:
        return ascii;
    }
}

//  0:      error
//  ESC_1B: eat escape char
wchar_t escape_sequence::add_char(char c)
{
    if(c == ESC_1B) { in_esc = true; return ESC_1B; }
    if(!in_esc)
    {
        if(c == SI_LS0) { GL = 0; return ESC_1B; }
        else if(c == SO_LS1) { GL = 1; return ESC_1B; }
        else return translate(c);
    }
    // c is i1
    if(i1 == '\0')
    {   /* fh is:
        02                       nF              ָ���ּ� & �õ���Щ����
        03                       Fp              private control function
        04��05                   Fe              C1��control character
        06��07(07/15����)        Fs              �˜��e�Ć�һ������Ԫ      */
        char ch = c & 0xf0;
        if(ch != 0x20)
        {
            reset();
            switch(c)
            {
            case ESC_LS2:   GL = 2; break;
            case ESC_LS3:   GL = 3; break;
            case ESC_SS2_7b:
            case ESC_SS2_8b:SS = 2; break;
            case ESC_SS3_7b:
            case ESC_SS3_8b:SS = 3; break;
            case ESC_LS1R:  GR = 1; break;
            case ESC_LS2R:  GR = 2; break;
            case ESC_LS3R:  GR = 3; break;
            default: break; // ignore other i1
            }
        }
        else i1 = c;
        return ESC_1B;
    }
    /* i1 is 02/xx, c is i2 or f
    nF�N�   ��һ��I byte   �ڶ���I byte   ������һ�Nfunction
    ---------------------------------------------------------------------
      0F        02/00           N           ָ��Ҫ����Щ����
      1F        02/01           O           ָ��C0�ּ�
      2F        02/02           O           ָ��C1�ּ�
      3F        02/03           O           ��һ������Ԫ
      4F        02/04           Y           ָ��multibyte character set
      5F        02/05           O           ���@�ݘ˜�֮���coding system
      6F        02/06           N           �ּ��İ汾
      7F        02/07           N           ����
      8F        02/08           O           ָ��һ��94-set�oG0
      9F        02/09           O           ָ��һ��94-set�oG1
     10F        02/10           O           ָ��һ��94-set�oG2
     11F        02/11           O           ָ��һ��94-set�oG3
     12F        02/12           N           ����
     13F        02/13           O           ָ��һ��96-set�oG1
     14F        02/14           O           ָ��һ��96-set�oG2
     15F        02/15           O           ָ��һ��96-set�oG3
    ---------------------------------------------------------------------- */
    switch(i1)
    {
    case 0x24:
        if(i2) break;
        if(c == 0x40 || c == 0x41 || c == 0x42)
        {
            G[0] = to_charset(0x2400 | c);
            reset();
            return ESC_1B;
        }
        // else 0x24 shall use i2, break
        break;
    case 0x28:
        G[0] = to_charset(c);
        reset();
        return ESC_1B;
    case 0x29:
    case 0x2d:
        G[1] = to_charset(c);
        reset();
        return ESC_1B;
    case 0x2a:
    case 0x2e:
        G[2] = to_charset(c);
        reset();
        return ESC_1B;
    case 0x2b:
    case 0x2f:
        G[3] = to_charset(c);
        reset();
        return ESC_1B;
    default: // 0x20 0x21 0x22 0x23 0x25 0x26 0x27 0x2c
        reset(); // ignore
        return ESC_1B;
    }

    //i1 == 0x24 and (c != 0x40 && c != 0x41 && c != 0x42), ָ��multibyte character set
    if(i2 == '\0')
    {
        i2 = c;
        return ESC_1B;
    }
    else // c is f
    {
        switch(i2)
        {
        case 0x28:
            G[0] = to_charset(c);
            reset();
            return ESC_1B;
        case 0x29:
        case 0x2d:
            G[1] = to_charset(c);
            reset();
            return ESC_1B;
        case 0x2a:
        case 0x2e:
            G[2] = to_charset(c);
            reset();
            return ESC_1B;
        case 0x2b:
        case 0x2f:
            G[3] = to_charset(c);
            reset();
            return ESC_1B;
        default: // 0x20 0x21 0x22 0x23 0x25 0x26 0x27 0x2c
            reset();// ignore
            break;  // error
        }
    }
    reset();
    return 0;
}
