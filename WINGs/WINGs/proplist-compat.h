

#ifndef _PROPLIST_COMPAT_H_
#define _PROPLIST_COMPAT_H_

#include <WINGs/WUtil.h>


typedef WMPropList* proplist_t;

#define PLSetCaseSensitive(c) WMPLSetCaseSensitive(c)

#define PLMakeString(bytes) WMCreatePLString(bytes)
#define PLMakeData(bytes, length) WMCreatePLDataFromBytes(bytes, length)
#define PLMakeArrayFromElements(pl, ...) WMCreatePLArray(pl, ...)
#define PLMakeDictionaryFromEntries(key, value, ...) WMCreatePLDictionary(key, value, ...)

#define PLRetain(pl) WMRetainPropList(pl)
#define PLRelease(pl) WMReleasePropList(pl)

#define PLInsertArrayElement(array, pl, pos) WMInsertInPLArray(array, pos, pl)
#define PLAppendArrayElement(array, pl) WMAddToPLArray(array, pl)
#define PLRemoveArrayElement(array, pos) WMDeleteFromPLArray(array, pos)
#define PLInsertDictionaryEntry(dict, key, value) WMPutInPLDictionary(dict, key, value)
#define PLRemoveDictionaryEntry(dict, key) WMRemoveFromPLDictionary(dict, key)
#define PLMergeDictionaries(dest, source) WMMergePLDictionaries(dest, source)

#define PLGetNumberOfElements(pl) WMGetPropListItemCount(pl)

#define PLIsString(pl) WMIsPLString(pl)
#define PLIsData(pl) WMIsPLData(pl)
#define PLIsArray(pl) WMIsPLArray(pl)
#define PLIsDictionary(pl) WMIsPLDictionary(pl)
#define PLIsSimple(pl) (WMIsPLString(pl) || WMIsPLData(pl))
#define PLIsCompound(pl) (WMIsPLArray(pl) || WMIsPLDictionary(pl))
#define PLIsEqual(pl1, pl2) WMIsPropListEqualTo(pl1, pl2)

#define PLGetString(pl) WMGetFromPLString(pl)
#define PLGetDataBytes(pl) WMGetPLDataBytes(pl)
#define PLGetDataLength(pl) WMGetPLDataLength(pl)
#define PLGetArrayElement(pl, index) WMGetFromArray(pl, index)
#define PLGetDictionaryEntry(pl, key) WMGetFromPLDictionary(pl, key)
#define PLGetAllDictionaryKeys(pl) WMGetPLDictionaryKeys(pl)

#define PLShallowCopy(pl) WMShallowCopyPropList(pl)
#define PLDeepCopy(pl) WMDeepCopyPropList(pl)

#define PLGetProplistWithDescription(desc) WMCreatePropListFromDescription(desc)
#define PLGetDescriptionIndent(pl, level) WMGetPropListDescription(pl, True)
#define PLGetDescription(pl) WMGetPropListDescription(pl, False)
#define PLGetStringDescription(pl) WMGetPropListDescription(pl, False)
#define PLGetDataDescription(pl) WMGetPropListDescription(pl, False)

#define PLGetProplistWithPath(file) WMReadPropListFromFile(file)
#define PLSave(pl, file, atm) WMWritePropListToFile(pl, file, atm)



//#define PLSetStringCmpHook(fn)
//#define PLDeepSynchronize(pl) PLDeepSynchronize_is_not_supported
//#define PLSynchronize(pl) PLSynchronize_is_not_supported
//#define PLShallowSynchronize(pl) error_PLShallowSynchronize_is_not_supported
//#define PLSetFilename(pl, filename) error_PLSetFilename_is_not_supported
//#define PLGetFilename(pl, filename) error_PLGetFilename_is_not_supported
//#define PLGetDomainNames()
//#define PLGetDomain(name)
//#define PLSetDomain(name, value, kickme)
//#define PLDeleteDomain(name, kickme)
//#define PLRegister(name, callback)
//#define PLUnregister(name)
//#define PLGetContainer(pl)


#endif
