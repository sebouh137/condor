use strict;
use warnings;

our %configuration;

sub materials
{	
	# Scintillator
	my %mat = init_mat();
	$mat{"name"}          = "scintillator";
	$mat{"description"}   = "fcal scintillator material";
	$mat{"density"}       = "1.032";
	$mat{"ncomponents"}   = "2";
	$mat{"components"}    = "C 9 H 10";	
	print_mat(\%configuration, \%mat);
  
  #my %mat2 = init_mat();
  #$mat2{"name"}          = "steel";
  #$mat2{"description"}   = "fcal absorber material";
  #$mat2{"density"}       = "7.85";  #typical steel density
  #$mat2{"ncomponents"}   = "7";
  #$mat2{"components"}    = "G4_Fe 98 G4_C 0.25 G4_Cu 0.2 G4_Mn 1.03 G4_P 0.04 G4_Si 0.28 G4_S 0.05";
  #print_mat(\%configuration, \%mat2);
	
}


