#ifndef UUID_A5D8D600_A5AA_42BA_8D07_E501F199E32B
#define UUID_A5D8D600_A5AA_42BA_8D07_E501F199E32B

#include "colors.hpp"

#define DEBUG true

#if DEBUG
#include <iostream>
#include <string.h>
// Pretty debug macros ;-;

#define PDEBUG(x) do {std::cout << Color::FG_LIGHT_GREEN << "DEBUG" \
	<< Color::FG_DEFAULT << ":" <<  Color::FG_LIGHT_BLUE \
	<< __FILE__ << Color::FG_DEFAULT << ":" \
	<< Color::FG_LIGHT_GREEN << __LINE__ << Color::FG_DEFAULT \
	<< ": " << x << std::endl; } while(0)

#else
#define PDEBUG(x)
#endif

#define PWARN(x) PCMSG(Color::FG_YELLOW, "WARNING", x)
#define PERR(x) PCMSG(Color::FG_RED, "ERROR", x)

#define PMSG(t, x) PCMSG(Color::FG_DEFAULT, t, x)

#define PCMSG(c, t, x) do {std::cout << c << t \
	<< Color::FG_DEFAULT << ": " << x << std::endl; } while(0)


#endif //UUID_A5D8D600_A5AA_42BA_8D07_E501F199E32B
