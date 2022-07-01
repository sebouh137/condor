use strict;
use warnings;

our %configuration;
our %parameters;

#units are meters
my $panel_length=1.6;
my $panel_width=0.6;
my $panel_thickness=0.01;

my $n_groups_x = 14;
my $n_groups_z = 14;

my $group_spacing_x = 8.7;
my $group_spacing_z = 8.08;

my $n_panels_x = 4;
my $n_panels_y = 2;
my $n_panels_z = 13;

my $panel_spacing_x = 2.000;
my $panel_spacing_y = 1.300;
my $panel_spacing_z =  0.604;

sub makeCONDOR
{
    build_mother();
    build_panels();
}

sub build_mother
{
    my %detector = init_det();
    
    $detector{"name"}        = "condor";
    $detector{"mother"}      = "root";
    $detector{"description"} = "CONDOR Mother Volume";
    $detector{"pos"}         = "0*m 0*m 0*m";
    $detector{"rotation"}    = "0*deg 0*deg 0*deg";
    $detector{"color"}       = "000000";
    $detector{"type"}        = "Box";
    $detector{"dimensions"}  = "130*m 20*m 120*m";
    $detector{"material"}    = "G4_AIR";
    $detector{"mfield"}      = "no";
    $detector{"visible"}     = 0;
    $detector{"style"}       = 0;
    print_det(\%configuration, \%detector);
}

sub build_panels()
{
  for(my $gi=1; $gi<=$n_groups_x; $gi++)
  {
    my $str_gi     = cnumber($gi-1, 10);
    
    for(my $gj=1; $gj<=$n_groups_z; $gj++)
    {
      #skip some of the groups near the edge.
      next if ($gi==2 or $gi==$n_groups_x-1 or $gj==2 or $gj==$n_groups_z-1 or
        (($gi==1 or $gi==$n_groups_x) and ($gj==2 or $gj==4 or $gj==5 or $gj==7 or $gj==8 or $gj==10 or $gj==11 or $gj==13)) or
        (($gj==1 or $gj==$n_groups_z) and ($gi==2 or $gi==4 or $gi==5 or $gi==7 or $gi==8 or $gi==10 or $gi==11 or $gi==13)));
      my $str_gj     = cnumber($gj-1, 10);
      my %detector = init_det();
      $detector{"name"}        = "panelgroup_${str_gi}_${str_gj}";
      $detector{"mother"}      = "condor" ;
      $detector{"description"} = "panel group ${str_gi} ${str_gj}";

      # positioning

      my $x      = sprintf("%.5f", $group_spacing_x*($gi-1/2-$n_groups_x/2));
      my $y      = 0;
      #my $y      = sprintf("%.3f", $SPACING_Y*$n);
      my $z      = sprintf("%.5f", $group_spacing_z*($gj-1/2-$n_groups_z/2));
      
      my $dx = $group_spacing_x/2;
      my $dy = 1;
      my $dz = $group_spacing_z/2;
      $detector{"pos"}        = "$x*m $y*m $z*m";
      $detector{"rotation"}   = "0*deg 0*deg 0*deg";
      $detector{"color"}      = "66bbff";
      $detector{"type"}       = "Box";
      $detector{"dimensions"} = "$dx*m $dy*m  $dz*m";
      $detector{"material"}   = "G4_AIR";
      $detector{"visible"}     = 0;
      $detector{"style"}       = 0;
      

      print_det(\%configuration, \%detector);
      
      #for(my $pi=1; $pi<=0; $pi++)
      for(my $pi=1; $pi<=$n_panels_x; $pi++)
      {
        for(my $pj=1; $pj<=$n_panels_y; $pj++)
        {
          for(my $pk=1; $pk<=$n_panels_z; $pk++)
          {
            my %detector = init_det();
	    
            $detector{"name"}        = "scint_${str_gi}_${str_gj}_${pi}_${pj}_${pk}";
            $detector{"mother"}      = "panelgroup_${str_gi}_${str_gj}" ;
            $detector{"description"} = "Condor Scintillator number ${str_gi} ${str_gj} ${pi} ${pj} ${pk}";
	    
            # positioning

            my $x  = sprintf("%.5f", $panel_spacing_x*($pi-1/2-$n_panels_x/2));
            my $y  = sprintf("%.5f", $panel_spacing_y*($pj-1/2-$n_panels_y/2));
            my $z  = sprintf("%.5f", $panel_spacing_z*($pk-1/2-$n_panels_z/2));
	    
            my $dx = $panel_length/2;
            my $dy = $panel_thickness/2;
            my $dz = $panel_width/2;
            
            $detector{"pos"}        = "$x*m $y*m $z*m";
            $detector{"rotation"}   = "0*deg 0*deg 0*deg";
            $detector{"color"}      = "66bbff";
            $detector{"type"}       = "Box";
            $detector{"dimensions"} = "$dx*m $dy*m $dz*m";
            $detector{"material"}   = "scintillator";
            $detector{"visible"}     = 1;
            $detector{"style"}       = 1;
            $detector{"sensitivity"} = "flux";
            $detector{"hit_type"}    = "flux";
            $detector{"identifiers"} = "group_row manual $gj group_col manual $gj panel_x manual $pi panel_y manual $pj panel_z manual $pk side manual 0";
	    
            print_det(\%configuration, \%detector);
            
          }
        }
      }
    }
  }
}


