#include "nmt/enums.h"

bool isMemberEntityKind(EntityKind ek) {
    switch (ek) {
        case EntityKind::enum_:
        case EntityKind::fn:
        case EntityKind::struct_:
        case EntityKind::class_:
        case EntityKind::header:
            return false;
        case EntityKind::memfn:
            return true;
    }
}
