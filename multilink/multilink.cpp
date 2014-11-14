#include "multilink.h"

namespace Multilink {
    Multilink::Multilink() {

    }

    void Link::display(std::ostream& stream) const {
        stream << "Link " << name << " rtt=" << std::endl;
    }
}
