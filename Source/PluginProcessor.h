/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope {
	Slope_12,
	Slope_24,
	Slope_36,
	Slope_48
};

struct ChainSettings {
	float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
	float lowCutFreq{ 0 }, highCutFreq{ 0 };
	Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class SimpleEQAudioProcessor : public juce::AudioProcessor
{
public:
	//==============================================================================
	SimpleEQAudioProcessor();
	~SimpleEQAudioProcessor() override;

	//==============================================================================

	// Prepare to play is called before the audio processing begins(setting up the filters, buffers, DSP)
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	// Release resources is called when playback stops, or the processor is being deleted
	void releaseResources() override;

	//	Tells JUCE which audio configs are supported by the plugin
#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif
	// The main processing function, called continuously by DAW/host
	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	//==============================================================================
	// Creates the editor component that will be shown to the user
	juce::AudioProcessorEditor* createEditor() override;

	// Indicates whether the plugin has the GUI
	bool hasEditor() const override;

	//==============================================================================
	// These methods provide information about the plugin (metadata)
	const juce::String getName() const override;
	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	//==============================================================================
	// These methods manage preset programs (not used in this simple EQ -> returns default)
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const juce::String getProgramName(int index) override;
	void changeProgramName(int index, const juce::String& newName) override;

	//==============================================================================
	// These methods handle saving and loading of plugin state
	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;

	// Method to create the parameter layout
	static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

	// This is the object that will hold and manage all our parameters
	juce::AudioProcessorValueTreeState apvts{ *this, nullptr,  "Parameters", createParameterLayout() };

private:
	using Filter = juce::dsp::IIR::Filter<float>;

	using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

	using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

	MonoChain leftChain, rightChain;

	enum ChainPositions
	{
		LowCut,
		Peak,
		HighCut
	};

	void updatePeakFilter(const ChainSettings& chainSettings);


	//	Shorter name for the pointer type that stores biquad filter coefficients
	using Coefficients = Filter::CoefficientsPtr;

	//	Helper function which replaces one coefficient pointer with another
	static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

	template<int Index, typename ChainType, typename CoefficientType>
	void update(ChainType& chain, const CoefficientType& cutCoefficients) {

		updateCoefficients(chain.template get<Index>().coefficients, cutCoefficients[Index]);
		chain.template setBypassed<Index>(false);
	}

	template<typename ChainType, typename CoefficientType>
	void updateCutFilter(ChainType& leftLowCut, const CoefficientType& cutCoefficients, const Slope& lowCutSlope) {
		leftLowCut.template setBypassed<0>(true);
		leftLowCut.template setBypassed<1>(true);
		leftLowCut.template setBypassed<2>(true);
		leftLowCut.template setBypassed<3>(true);

		switch (lowCutSlope) {

		case Slope_48: {
			update<3>(leftLowCut, cutCoefficients);
		}
		case Slope_36: {
			update<2>(leftLowCut, cutCoefficients);

		}
		case Slope_24: {
			update<1>(leftLowCut, cutCoefficients);

		}
		case Slope_12: {
			update<0>(leftLowCut, cutCoefficients);
		}
		}
	}

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEQAudioProcessor)
};
