use strict;
use warnings;

our %configuration;

# Variable Type is two chars.
# The first char:
#  R for raw integrated variables
#  D for dgt integrated variables
#  S for raw step by step variables
#  M for digitized multi-hit variables
#  V for voltage(time) variables
#
# The second char:
# i for integers
# d for doubles

my $bankId   = 900;
my $bankname = "condor";

sub define_bank
{
	
	# uploading the hit definition
	insert_bank_variable(\%configuration, $bankname, "bankid",   $bankId, "Di", "$bankname bank ID");
	insert_bank_variable(\%configuration, $bankname, "group_x",       1, "Di", "column number for the panel group");
	insert_bank_variable(\%configuration, $bankname, "group_y",      2, "Di", "row number for the panel group");
	insert_bank_variable(\%configuration, $bankname, "panel_x",      3, "Di", "x index for the panel within the group");
        insert_bank_variable(\%configuration, $bankname, "panel_y",      4, "Di", "y index for the panel within the group");
        insert_bank_variable(\%configuration, $bankname, "panel_z",      5, "Di", "z index for the panel within the group");
	insert_bank_variable(\%configuration, $bankname, "ADC_order",    6, "Di", "side: 0 - ADCL , 1 - ADCR");
	insert_bank_variable(\%configuration, $bankname, "ADC_ADC",      7, "Di", "ADC integral from pulse fit");
	insert_bank_variable(\%configuration, $bankname, "ADC_time" ,    8, "Dd", "time from pulse fit");
	insert_bank_variable(\%configuration, $bankname, "ADC_ped" ,     9, "Di", "pedestal from pulse analysis");
	insert_bank_variable(\%configuration, $bankname, "TDC_order",    10, "Di", "side: 2 - TDCL , 3 - TDCR");
	insert_bank_variable(\%configuration, $bankname, "TDC_TDC",      11, "Di", "TDC value");
	insert_bank_variable(\%configuration, $bankname, "hitn",        99, "Di", "hit number");
}
