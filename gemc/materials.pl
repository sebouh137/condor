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
  
  	my %mat2 = init_mat();
  	$mat2{"name"}          = "dirt";
  	$mat2{"description"}   = "soil";
  	$mat2{"density"}       = "1.5";  #typical dirt density
  	$mat2{"ncomponents"}   = "2";
  	$mat2{"components"}    = "O 53 Si 47"; # formula for silicon dioxide. 
  	print_mat(\%configuration, \%mat2);
	
}


