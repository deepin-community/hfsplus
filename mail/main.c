
#include <stdio.h>

#define __USE_GNU

#include <wchar.h>

int main(int arc, char* argv[])
{
    mbstate_t mbs = { 0, 0};
    wchar_t sw1 [] ={ 65, 776, 79, 776, 85, 776, 187, 8222, 8240, 184, 733, 711, 
		      65, 769, 85, 770, 216, 8719, 65, 778, 73, 769, 8364, 73, 776,
		      73, 768, 79, 769, 305, 710, 64258, 8225, 85, 768, 67, 807, 9674,
		      8249, 8250, 728, 731, 247, };
    wchar_t sw2 [] ={ 97, 776, 111, 776, 117, 776, 223, 171, 8721, 8482, 174, 8224, 937,
		      168, 8260, 248, 960, 97, 778, 8218, 8706, 402, 169, 170, 186, 8710,
		      172, 165, 8776, 99, 807, 8730, 8747, 126, 181, 8734 };
    const wchar_t *psw = sw1;
    char    s [64];
    int	    res = wcsnrtombs(s, &psw, 43, 64,  &mbs);
    s[res] = '\0';
    printf("%d %s \n", res, s);
    
    psw = sw2;
    res = wcsnrtombs(s, &psw, 36, 64,  &mbs);
    s[res] = '\0';
    printf("%d %s \n", res, s);
    return 0;
}
