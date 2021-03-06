#include "JDDeviceManager.h"
#include "JDControlService.h"
#include "CodalDmesg.h"

using namespace codal;

int JDDeviceManager::initialiseDevice(JDDevice* remoteDevice, JDControlPacket* controlPacket, uint8_t communicationRate)
{
    remoteDevice->device_address = controlPacket->device_address;
    remoteDevice->unique_device_identifier = controlPacket->unique_device_identifier;
    remoteDevice->device_flags = controlPacket->device_flags;
    remoteDevice->communication_rate = communicationRate;
    remoteDevice->rolling_counter = 0;

    if (controlPacket->device_flags & JD_DEVICE_FLAGS_HAS_NAME)
    {
        uint8_t len = *controlPacket->data;

        if (remoteDevice->name)
        {
            if (len == strlen((char *)remoteDevice->name) && memcmp(controlPacket->data + 1, remoteDevice->name, len))
                return DEVICE_OK;

            free(remoteDevice->name);
            remoteDevice->name = NULL;
        }

        remoteDevice->name = (uint8_t *)malloc(len + 1);
        memcpy(remoteDevice->name, controlPacket->data + 1, len);
        remoteDevice->name[len] = 0;
    }

    return DEVICE_OK;
}

JDDeviceManager::JDDeviceManager()
{
    this->devices = NULL;
}

JDDevice* JDDeviceManager::getDeviceList()
{
    return this->devices;
}

JDDevice* JDDeviceManager::getDevice(uint8_t device_address)
{
    JDDevice* head = this->devices;

    while(head)
    {
        if (head->device_address == device_address)
            return head;

        head = head->next;
    }

    return NULL;
}

JDDevice* JDDeviceManager::getDevice(uint8_t device_address, uint64_t unique_device_identifier)
{
    JDDevice* head = this->devices;

    while(head)
    {
        if (head->device_address == device_address && head->unique_device_identifier == unique_device_identifier)
            return head;

        head = head->next;
    }

    return NULL;
}

JDDevice* JDDeviceManager::addDevice(JDControlPacket* controlPacket, uint8_t communicationRate)
{
    JDDevice* newRemote = (JDDevice *) malloc(sizeof(JDDevice));

    newRemote->next = NULL;
    newRemote->name = NULL;
    newRemote->servicemap_bitmsk = 0;

    initialiseDevice(newRemote, controlPacket, communicationRate);

    if (this->devices == NULL)
        this->devices = newRemote;
    else
    {
        JDDevice* head = this->devices;
        JDDevice* prev = this->devices;

        while(head)
        {
            // guard against duplicates.
            if (head->device_address == newRemote->device_address && head->unique_device_identifier == newRemote->unique_device_identifier)
            {
                if (newRemote->device_flags & JD_DEVICE_FLAGS_HAS_NAME)
                    free (newRemote->name);

                free(newRemote);
                return head;
            }

            prev = head;
            head = head->next;
        }

        prev->next = newRemote;
    }

    return newRemote;
}

int JDDeviceManager::updateDevice(JDDevice* remoteDevice, JDControlPacket* controlPacket, uint8_t communicationRate)
{
    return initialiseDevice(remoteDevice, controlPacket, communicationRate);
}

int JDDeviceManager::removeDevice(JDDevice* device)
{
    if (this->devices == NULL)
        return DEVICE_INVALID_PARAMETER;

    JDDevice* curr = this->devices;
    JDDevice* prev = NULL;

    if (device == curr)
        this->devices = curr->next;
    else
    {
        prev = curr;
        curr = curr->next;

        while (curr)
        {
            // found!!
            if (curr->device_address == device->device_address && curr->unique_device_identifier == device->unique_device_identifier)
            {
                prev->next = curr->next;
                return DEVICE_OK;
            }

            prev = curr;
            curr = curr->next;
        }
    }

    return DEVICE_INVALID_PARAMETER;
}