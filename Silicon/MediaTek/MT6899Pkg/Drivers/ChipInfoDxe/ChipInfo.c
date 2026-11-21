/**
  Minimal MT6899 ChipInfo protocol producer.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/EFIChipInfo.h>

STATIC
EFI_STATUS
EFIAPI
Mt6899GetChipId (
  OUT UINT64  *Id
  )
{
  if (Id == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Id = 0x6899;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
Mt6899GetChipRevision (
  OUT UINT8  *MajorRev,
  OUT UINT8  *MinorRev
  )
{
  if (MajorRev == NULL || MinorRev == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *MajorRev = 0;
  *MinorRev = 0;
  return EFI_SUCCESS;
}

STATIC EFI_CHIP_INFO_PROTOCOL  mChipInfo = {
  Mt6899GetChipId,
  Mt6899GetChipRevision
};

EFI_STATUS
EFIAPI
RegisterChipInfo (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiChipInfoProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mChipInfo
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MT6899: failed to install ChipInfo protocol: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "MT6899: ChipInfo protocol installed\n"));
  return EFI_SUCCESS;
}
