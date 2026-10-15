#pragma once

namespace rinmotiondance {
namespace panel {

constexpr int kUndoButton = 400;             
constexpr int kRedoButton = 401;             
constexpr int kPlayButton = 408;             
constexpr int kPlayStartFrameEdit = 409;     
constexpr int kPlayStopFrameEdit = 410;      
constexpr int kCurrentFrameEdit = 417;       
constexpr int kRangeStartEdit = 425;         
constexpr int kRangeStopEdit = 426;          
constexpr int kTimelineVScroll = 427;        
constexpr int kTimelineHScroll = 428;        
constexpr int kPasteButton = 421;            
constexpr int kReversePasteButton = 422;     
constexpr int kExpandShrinkButton = 424;     
constexpr int kCurveCopyButton = 430;        
constexpr int kCurvePasteButton = 431;       
constexpr int kCurveLinearButton = 432;      

constexpr int kCameraRefModelCheckbox = 412; 
constexpr int kInterpCurveCombo = 433;       
constexpr int kRegisterScopeCombo = 434;     
constexpr int kMainComboModel = 436;         
constexpr int kModelVisibleCheckbox = 439;   
constexpr int kShadowCheckbox = 440;         
constexpr int kAddBlendCheckbox = 441;       
constexpr int kIkChainCombo = 443;           
constexpr int kIkOnRadio = 444;              
constexpr int kIkOffRadio = 445;             
constexpr int kPerspectiveCheckbox = 446;    
constexpr int kFovSlider = 447;              
constexpr int kFovEdit = 448;                
constexpr int kMainComboNormal = 449;        
constexpr int kBoneRegisterCombo = 450;      
constexpr int kLightColorSliderR = 455;      
constexpr int kLightColorSliderG = 456;      
constexpr int kLightColorSliderB = 457;      
constexpr int kLightDirSliderX = 458;        
constexpr int kLightDirSliderY = 459;        
constexpr int kLightDirSliderZ = 460;        
constexpr int kLightColorEditR = 461;        
constexpr int kLightColorEditG = 462;        
constexpr int kLightColorEditB = 463;        
constexpr int kLightDirEditX = 464;          
constexpr int kLightDirEditY = 465;          
constexpr int kLightDirEditZ = 466;          

constexpr int kAccessoryCombo = 471;         
constexpr int kMainComboGround = 474;        
constexpr int kAttachBoneCombo = 475;        
constexpr int kAccessoryVisibleCheckbox = 476;  
constexpr int kAccessoryAddBlendCheckbox = 477; 
constexpr int kAccPosXEdit = 478;            
constexpr int kAccPosYEdit = 479;            
constexpr int kAccPosZEdit = 480;            
constexpr int kAccRotXEdit = 481;            
constexpr int kAccRotYEdit = 482;            
constexpr int kAccRotZEdit = 483;            
constexpr int kAccScaleXEdit = 484;          
constexpr int kAccScaleYEdit = 485;          
constexpr int kAccessoryShadowCheckbox = 486;   

constexpr int kBoneSelectRadio = 490;        
constexpr int kBoxSelectRadio = 491;         
constexpr int kBoneMoveRadio = 492;          
constexpr int kBoneRotateRadio = 493;        
constexpr int kBonePasteButton = 497;        
constexpr int kBoneReversePasteButton = 498; 
constexpr int kPhysicsCheckbox = 499;        
constexpr int kRegisterFrameButton = 500;    

constexpr int kMorphCombo0 = 504;            
constexpr int kMorphSlider0 = 505;           
constexpr int kMorphEdit0 = 506;             
constexpr int kMorphCombo1 = 509;            
constexpr int kMorphSlider1 = 510;           
constexpr int kMorphEdit1 = 511;             
constexpr int kMorphCombo2 = 514;            
constexpr int kMorphSlider2 = 515;           
constexpr int kMorphEdit2 = 516;             
constexpr int kMorphCombo3 = 519;            
constexpr int kMorphSlider3 = 520;           
constexpr int kMorphEdit3 = 521;             

constexpr int kPhysicsFrameCheckbox = 530;   
constexpr int kCameraRefBoneCheckbox = 531;  
constexpr int kFrameVolumeSlider = 534;      
constexpr int kFollowCameraCheckbox = 535;   
constexpr int kModelEditToggle = 536;        
constexpr int kCameraDistanceButton = 543;   
constexpr int kReadoutPosXEdit = 544;        
constexpr int kReadoutPosYEdit = 545;        
constexpr int kReadoutPosZEdit = 546;        
constexpr int kReadoutRotXEdit = 547;        
constexpr int kReadoutRotYEdit = 548;        
constexpr int kReadoutRotZEdit = 549;        
constexpr int kReadoutDistEdit = 550;        
constexpr int kInfoCheckbox = 551;           
constexpr int kFrameVolumeCheckbox = 552;    
constexpr int kGotoFrameEdit = 554;          
constexpr int kSelfShadowCheckbox = 556;     
constexpr int kCoordAxisCheckbox = 557;      
constexpr int kSelfShadowRangeSlider = 560;  
constexpr int kSelfShadowRangeEdit = 561;    
constexpr int kEditOffCheckbox = 562;        
constexpr int kEditMode1Checkbox = 563;      
constexpr int kEditMode2Checkbox = 564;      

constexpr int kBiasXEdit = 601;              
constexpr int kBiasYEdit = 602;              
constexpr int kBiasZEdit = 603;              

constexpr int kScaleRateEdit = 605;          
constexpr int kScaleFromEdit = 686;          
constexpr int kScaleToEdit = 687;            
constexpr int kScaleBoneCheckbox = 688;      
constexpr int kScaleMorphCheckbox = 689;     
constexpr int kScaleDispIkCheckbox = 690;    

constexpr int kScreenWidthEdit = 621;        
constexpr int kScreenHeightEdit = 622;       

constexpr int kAviStartFrameEdit = 609;      
constexpr int kAviEndFrameEdit = 610;        
constexpr int kAviFpsEdit = 611;             
constexpr int kAviWidthEdit = 612;           
constexpr int kAviHeightEdit = 613;          
constexpr int kAviWaveCheckbox = 614;        
constexpr int kAviCodecCombo = 628;          
constexpr int kAviSettingsButton = 724;      
constexpr int kAvi3dVisionCheckbox = 806;    
constexpr int kAviLeftHalfCheckbox = 807;    

constexpr int kShiftFramesEdit = 616;        

constexpr int kBlinkStartEdit = 618;         
constexpr int kBlinkEndEdit = 619;           

constexpr int kGroundShadowSlider = 625;     
constexpr int kGroundShadowEdit = 626;       

constexpr int kOrderListBox = 628;           
constexpr int kAccessoryNameEdit = 629;      
constexpr int kOrderUpButton = 630;          
constexpr int kOrderDownButton = 631;        
constexpr int kDialogOkButton = 632;         
constexpr int kAccessoryOrderEdit = 635;     

constexpr int kNumInputPosXEdit = 637;       
constexpr int kNumInputPosYEdit = 638;       
constexpr int kNumInputPosZEdit = 639;       
constexpr int kNumInputRotXEdit = 640;       
constexpr int kNumInputRotYEdit = 641;       
constexpr int kNumInputRotZEdit = 642;       
constexpr int kNumInputDistEdit = 644;       

constexpr int kEdgeThicknessEdit = 646;      
constexpr int kEdgeThicknessSlider = 647;    

constexpr int kModelNameEnEdit = 667;        
constexpr int kModelCommentEnEdit = 668;     
constexpr int kBoneNameCombo = 669;          
constexpr int kBonePrevButton = 671;         
constexpr int kBoneNextButton = 670;         
constexpr int kBoneNameEnEdit = 672;         
constexpr int kMorphNameCombo = 673;         
constexpr int kMorphNameEnEdit = 676;        
constexpr int kGroupNameCombo = 677;         
constexpr int kGroupNameEnEdit = 680;        

constexpr int kGravityAccelEdit = 709;       
constexpr int kGravityDirXEdit = 710;        
constexpr int kGravityDirYEdit = 711;        
constexpr int kGravityDirZEdit = 712;        
constexpr int kGravityNoiseEdit = 713;       
constexpr int kBodyACombo = 741;             
constexpr int kBodyBCombo = 742;             
constexpr int kGravityNoiseCheckbox = 731;   
constexpr int kGravityDirXSlider = 637;      
constexpr int kGravityDirYSlider = 638;      
constexpr int kGravityDirZSlider = 639;      

constexpr int kToon01Edit = 709;             
constexpr int kToon02Edit = 710;             
constexpr int kToon03Edit = 711;             
constexpr int kToon04Edit = 712;             
constexpr int kToon05Edit = 713;             

constexpr int kCamMulPosXScaleEdit = 686;    
constexpr int kCamMulPosXOffsetEdit = 687;   
constexpr int kBoneMulPosXScaleEdit = 686;   
constexpr int kBoneMulPosXOffsetEdit = 687;  
constexpr int kMorphMulScaleEdit = 686;      
constexpr int kMorphMulOffsetEdit = 687;     

}  
}  
