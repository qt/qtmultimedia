######################################################################
#
# Mobility API project - Symbian backends
#
######################################################################

TEMPLATE = subdirs

include (../../../config.pri)

# The openmax-al backend is currently not supported
# we include mmf only if we are not building openmaxal based backend
#contains(openmaxal_symbian_enabled, no) {
#    message("Enabling mmf mediarecording, playback and radio backend")
#   symbian:SUBDIRS += mmf 
#
#else {
#   message("Enabling OpenMAX AL audio record, playback and radio backend")
#   symbian:SUBDIRS += openmaxal
#

symbian:SUBDIRS += ecam mmf 

