#include "enums.h"

NeedsKind NeedsKindForLightDeclaration(EntityKind ek){
	switch(ek){
		case EntityKind::enum_:
			return NeedsKind::opaqueEnumDeclaration;
		case EntityKind::struct_:
		case EntityKind::class_:
			return NeedsKind::forwardDeclaration;
		case EntityKind::fn:
		case EntityKind::using_:
		case EntityKind::inlvar:
		case EntityKind::memfn:
			CHECK(false);
	}
}

